// ============================================================
//  yalex_reader.cpp — Lector universal de especificaciones léxicas
//
//  Formatos soportados:
//  ┌─────────────────────────────────────────────────────────┐
//  │ .yalex   { let x = pat } rule tokens = pat { TOK } ...  │
//  │ .yal     let x = pat  rule tokens = parse  | pat { TOK }│
//  │ .lex/.l  name pat\n%%\npat { return TOK; }\n%%          │
//  └─────────────────────────────────────────────────────────┘
//
//  Comentarios aceptados:
//    (* ... *)   OCamllex / yalex estándar
//    // ...      C++ / yacc estilo
//    # ...       shell / Python estilo
//    /* ... */   C / flex estilo
//
//  Acciones aceptadas:
//    { TOKEN }              formato .yalex
//    { TOKEN; }             con punto y coma
//    { return TOKEN; }      formato flex
//    { return TOKEN }       flex sin ;
//    { TOKEN(arg) }         OCamllex con argumento — extrae TOKEN
//    { rule_name lexbuf }   OCamllex recursivo — descarta (SKIP)
//    { (* skip *) }         OCamllex skip
//    { /* skip */ }         flex skip
//    { }                    vacío — SKIP
// ============================================================
#include "yalex/yalex_reader.hpp"
#include <fstream>
#include <sstream>
#include <cctype>
#include <algorithm>

namespace yalex {

// Forward declaration de función libre
static std::string normalizeCharClassFlex(const std::string& pat);

// ════════════════════════════════════════════════════════════
//  Entrada pública
// ════════════════════════════════════════════════════════════
YAlexSpec YAlexReader::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) throw std::runtime_error("No se pudo abrir: " + filepath);
    std::ostringstream ss; ss << file.rdbuf();
    return readString(ss.str(), filepath);
}

YAlexSpec YAlexReader::readString(const std::string& rawContent,
                                   const std::string& sourceName) {
    // ── Quitar BOM UTF-8 (\xEF\xBB\xBF) si está presente ────────
    std::string content = rawContent;
    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF) {
        content = content.substr(3);
    }

    source_     = content;
    pos_        = 0;
    lineNum_    = 1;
    sourceName_ = sourceName;

    YAlexSpec spec;
    spec.sourceFile = sourceName;

    skipWhitespaceAndComments();
    if (atEnd()) error("Archivo vacío");

    // ── Detectar formato ─────────────────────────────────────────
    //  '{'            → .yalex  (bloque de macros entre llaves)
    //  "let"          → .yal    (lets al nivel raíz, OCamllex)
    //  "%%"           → .lex    (flex, sin sección de definiciones)
    //  letter/digit   → .lex    (flex con sección de definiciones)
    //  '%'            → flex con directivas o %{ preamble %}
    // ────────────────────────────────────────────────────────────
    char first = current();

    if (first == '{') {
        // ── .yalex ──────────────────────────────────────────────
        parseMacroBlock(spec);
        skipWhitespaceAndComments();
        parseRuleHeader(spec);

    } else if (first == '%') {
        // ── flex con directivas o %{ preamble %} ────────────────
        parseFlexPreamble(spec);
        skipWhitespaceAndComments();
        // Ahora deberíamos estar en '%%' o en 'rule'
        if (!atEnd() && current() == '%' &&
            pos_+1 < source_.size() && source_[pos_+1] == '%') {
            pos_ += 2; // consume %%
            parseFlexRules(spec);
        } else {
            parseRuleHeader(spec);
        }

    } else {
        // ── .yal / flex sin directivas ───────────────────────────
        // Puede tener "let" al nivel raíz o macros flex (nombre valor)
        // Ambos terminan cuando encontramos "rule" o "%%"
        parseTopLevelDefs(spec);
        skipWhitespaceAndComments();

        if (!atEnd() && current() == '%' &&
            pos_+1 < source_.size() && source_[pos_+1] == '%') {
            pos_ += 2; // consume %%
            parseFlexRules(spec);
        } else if (!atEnd()) {
            parseRuleHeader(spec);
        }
    }

    if (spec.rules.empty()) error("No se encontraron reglas léxicas");
    return spec;
}

// ════════════════════════════════════════════════════════════
//  parseFlexPreamble — salta %{...%} y %token/%option/etc.
// ════════════════════════════════════════════════════════════
void YAlexReader::parseFlexPreamble(YAlexSpec& spec) {
    while (!atEnd()) {
        skipWhitespaceAndComments();
        if (atEnd()) break;
        char c = current();

        if (c == '%') {
            // ¿Es '%%' (separador)?
            if (pos_+1 < source_.size() && source_[pos_+1] == '%') break;
            // ¿Es '%{' (bloque de código)?
            if (pos_+1 < source_.size() && source_[pos_+1] == '{') {
                skipPreambleBlock();
                continue;
            }
            // Otro % → directiva desconocida, saltar línea
            skipToEndOfLine();
            continue;
        }

        if (c == 'r' || c == 'R') {
            // Peek "rule" → salimos para que parseRuleHeader lo consuma
            size_t sv = pos_; int sl = lineNum_;
            std::string w = readWord();
            if (w == "rule") { pos_ = sv; lineNum_ = sl; break; }
            pos_ = sv; lineNum_ = sl;
        }

        // Definición flex: nombre<espacio>patrón  (sin '=')
        // Ejemplo: DIGIT [0-9]
        if (isalpha(c) || c == '_') {
            size_t sv = pos_; int sl = lineNum_;
            std::string name = readWord();
            skipWS(); // solo espacios/tabs, no newline
            if (!atEnd() && current() != '\n' && current() != '\r') {
                std::string pattern = readUntilEndOfLine();
                trimInPlace(pattern);
                if (!pattern.empty()) {
                    spec.macros[name] = normalizeCharClassFlex(pattern);
                    continue;
                }
            }
            // Si no hay patrón en la misma línea, no era macro → retroceder
            pos_ = sv; lineNum_ = sl;
            skipToEndOfLine();
            continue;
        }

        skipToEndOfLine();
    }
}

// ── Salta un bloque %{ ... %} ─────────────────────────────────
void YAlexReader::skipPreambleBlock() {
    // pos_ apunta a '%'
    pos_ += 2; // salta '%{'
    while (!atEnd()) {
        if (current() == '%' && pos_+1 < source_.size() && source_[pos_+1] == '}') {
            pos_ += 2; return;
        }
        if (current() == '\n') lineNum_++;
        pos_++;
    }
}

// ════════════════════════════════════════════════════════════
//  parseTopLevelDefs — "let name = pat" o "name pat" (flex)
//  hasta encontrar "rule" o "%%"
// ════════════════════════════════════════════════════════════
void YAlexReader::parseTopLevelDefs(YAlexSpec& spec) {
    while (!atEnd()) {
        skipWhitespaceAndComments();
        if (atEnd()) break;
        char c = current();

        // "%%"  → separador de sección
        if (c == '%' && pos_+1 < source_.size() && source_[pos_+1] == '%') break;

        // "%{"  → bloque de código, saltarlo
        if (c == '%' && pos_+1 < source_.size() && source_[pos_+1] == '{') {
            skipPreambleBlock(); continue;
        }

        // Otro '%' → directiva, saltar línea
        if (c == '%') { skipToEndOfLine(); continue; }

        if (!isalpha(c) && c != '_') {
            // Caracter inesperado, saltar línea
            skipToEndOfLine(); continue;
        }

        size_t sv = pos_; int sl = lineNum_;
        std::string word = readWord();

        // "rule" → fin de macros
        if (word == "rule") { pos_ = sv; lineNum_ = sl; break; }

        skipWS();

        if (!atEnd() && current() == '=') {
            // Formato .yal: word = pattern
            if (word == "let") {
                // Siguiente palabra es el nombre
                skipWhitespaceAndComments();
                std::string name = readWord();
                skipWhitespaceAndComments();
                if (!atEnd() && current() == '=') { pos_++; }
                skipWS();
                std::string pat = readUntilEndOfLine(); trimInPlace(pat);
                if (!pat.empty()) spec.macros[name] = normalizeCharClassFlex(pat);
            } else {
                // "name = pattern"
                pos_++; skipWS();
                std::string pat = readUntilEndOfLine(); trimInPlace(pat);
                if (!pat.empty()) spec.macros[word] = normalizeCharClassFlex(pat);
            }
        } else if (word == "let") {
            // "let name = pattern" sin el '=' aún
            skipWhitespaceAndComments();
            std::string name = readWord();
            skipWhitespaceAndComments();
            if (!atEnd() && current() == '=') pos_++;
            skipWS();
            std::string pat = readUntilEndOfLine(); trimInPlace(pat);
            if (!pat.empty()) spec.macros[name] = normalizeCharClassFlex(pat);
        } else {
            // Definición flex sin '=': "NAME pattern" (en misma línea)
            if (!atEnd() && current() != '\n' && current() != '\r') {
                std::string pat = readUntilEndOfLine(); trimInPlace(pat);
                if (!pat.empty()) {
                    spec.macros[word] = normalizeCharClassFlex(pat);
                    continue;
                }
            }
            // Nada útil, saltar el resto de la línea
            skipToEndOfLine();
        }
    }
}

// ════════════════════════════════════════════════════════════
//  parseMacroBlock — { let name = pat ... }
// ════════════════════════════════════════════════════════════
void YAlexReader::parseMacroBlock(YAlexSpec& spec) {
    pos_++; // consume '{'
    skipWhitespaceAndComments();
    while (!atEnd() && current() != '}') {
        skipWhitespaceAndComments();
        if (atEnd() || current() == '}') break;

        char c = current();
        // Saltar líneas desconocidas en lugar de fallar
        if (!isalpha(c) && c != '_') { skipToEndOfLine(); continue; }

        std::string w = readWord();
        if (w == "let") {
            parseMacroDecl(spec);
        } else {
            // Formato sin "let": "name = pat"
            skipWS();
            if (!atEnd() && current() == '=') {
                pos_++; skipWS();
                std::string pat = readUntilEndOfLine(); trimInPlace(pat);
                if (!pat.empty()) spec.macros[w] = normalizeCharClassFlex(pat);
            } else {
                skipToEndOfLine();
            }
        }
        skipWhitespaceAndComments();
    }
    if (atEnd()) error("Bloque de macros sin cerrar (falta '}')");
    pos_++; // consume '}'
}

void YAlexReader::parseMacroDecl(YAlexSpec& spec) {
    skipWhitespaceAndComments();
    std::string name = readWord();
    if (name.empty()) error("Nombre de macro esperado");
    skipWhitespaceAndComments();
    if (!atEnd() && current() == '=') pos_++;
    skipWS();
    std::string pattern = readUntilEndOfLine();
    trimInPlace(pattern);
    if (pattern.empty()) error("Macro '" + name + "' vacía");
    spec.macros[name] = normalizeCharClassFlex(pattern);
}

// ════════════════════════════════════════════════════════════
//  parseRuleHeader — consume "rule name = [parse]"
// ════════════════════════════════════════════════════════════
void YAlexReader::parseRuleHeader(YAlexSpec& spec) {
    if (atEnd()) return;

    // Debe haber "rule"
    skipWhitespaceAndComments();
    std::string w = readWord();
    if (w != "rule") error("Se esperaba 'rule', encontrado: '" + w + "'");

    skipWhitespaceAndComments();
    ruleName_ = readWord(); // nombre de la regla (e.g. "tokens")
    skipWhitespaceAndComments();

    if (!atEnd() && current() == '=') {
        pos_++; // consume '='
        skipWhitespaceAndComments();
        // OCamllex tiene "rule name = parse"  → saltar "parse"
        size_t sv = pos_; int sl = lineNum_;
        std::string maybe = readWord();
        if (maybe != "parse") { pos_ = sv; lineNum_ = sl; }
        else skipWhitespaceAndComments();
    }

    parseRuleSection(spec);
}

// ════════════════════════════════════════════════════════════
//  parseRuleSection — lee reglas  pat { TOK } | pat { TOK } ...
// ════════════════════════════════════════════════════════════
void YAlexReader::parseRuleSection(YAlexSpec& spec) {
    skipWhitespaceAndComments();
    int priority = 0;

    // Primera regla puede o no tener '|' al frente
    if (!atEnd() && current() != '|') {
        auto r = tryParseOneRule(priority++);
        if (r.has_value()) spec.rules.push_back(*r);
    }

    skipWhitespaceAndComments();
    while (!atEnd()) {
        char c = current();
        // '%%' o segunda sección "rule" → terminar
        if (c == '%' && pos_+1 < source_.size() && source_[pos_+1] == '%') break;
        if (c == 'r') {
            size_t sv = pos_; int sl = lineNum_;
            std::string w = readWord();
            if (w == "rule") { pos_ = sv; lineNum_ = sl; break; }
            pos_ = sv; lineNum_ = sl;
        }
        if (c != '|') break;
        pos_++; // consume '|'
        skipWhitespaceAndComments();
        auto r = tryParseOneRule(priority++);
        if (r.has_value()) spec.rules.push_back(*r);
        skipWhitespaceAndComments();
    }

    // Expandir macros bare (formato .yal)
    for (auto& rule : spec.rules)
        rule.pattern = expandBareMacros(rule.pattern, spec.macros);
}

// ════════════════════════════════════════════════════════════
//  parseFlexRules — pat { return TOK; }  (sin '|' entre reglas)
// ════════════════════════════════════════════════════════════
void YAlexReader::parseFlexRules(YAlexSpec& spec) {
    skipWhitespaceAndComments();
    int priority = 0;
    while (!atEnd()) {
        // '%%' cierra la sección
        if (current() == '%' && pos_+1 < source_.size() && source_[pos_+1] == '%') break;
        skipWhitespaceAndComments();
        if (atEnd()) break;
        if (current() == '%' && pos_+1 < source_.size() && source_[pos_+1] == '%') break;

        auto r = tryParseOneRule(priority++);
        if (r.has_value()) spec.rules.push_back(*r);
        skipWhitespaceAndComments();
    }
    // Expandir macros bare
    for (auto& rule : spec.rules)
        rule.pattern = expandBareMacros(rule.pattern, spec.macros);
}

// ════════════════════════════════════════════════════════════
//  tryParseOneRule — intenta parsear una regla, devuelve nullopt
//  si la línea no parece una regla válida (en lugar de fallar)
// ════════════════════════════════════════════════════════════
std::optional<LexRule> YAlexReader::tryParseOneRule(int priority) {
    skipWhitespaceAndComments();
    if (atEnd()) return std::nullopt;

    size_t savedPos  = pos_;
    int    savedLine = lineNum_;

    try {
        LexRule r;
        r.priority   = priority;
        r.sourceLine = lineNum_;
        r.pattern    = readPattern();
        r.tokenName  = readTokenName();
        return r;
    } catch (...) {
        // Regla malformada: retroceder y saltar la línea
        pos_      = savedPos;
        lineNum_  = savedLine;
        skipToEndOfLine();
        return std::nullopt;
    }
}

// ════════════════════════════════════════════════════════════
//  readPattern — lee el patrón (hasta encontrar '{' de acción)
// ════════════════════════════════════════════════════════════
std::string YAlexReader::readPattern() {
    std::string pattern;
    int braceDepth = 0;

    while (!atEnd()) {
        char c = current();

        // String literal "..." — copiar completo, ignorar { } dentro
        if (c == '"') {
            pattern += c; pos_++;
            while (!atEnd() && current() != '"') {
                if (current() == '\\' && pos_+1 < source_.size()) {
                    pattern += current(); pos_++;
                }
                pattern += current(); pos_++;
            }
            if (!atEnd()) { pattern += current(); pos_++; } // cierra "

        // Char literal '.' — copiar completo, ignorar { } dentro
        } else if (c == '\'') {
            pattern += c; pos_++;
            while (!atEnd() && current() != '\'') {
                if (current() == '\\' && pos_+1 < source_.size()) {
                    pattern += current(); pos_++;
                }
                pattern += current(); pos_++;
            }
            if (!atEnd()) { pattern += current(); pos_++; } // cierra '

        } else if (c == '{') {
            if (braceDepth == 0 && !pattern.empty()) {
                // ¿Es la llave de acción o una referencia a macro?
                size_t look = pos_ + 1;
                while (look < source_.size() && source_[look] == ' ') look++;
                if (look < source_.size()) {
                    char nc = source_[look];
                    bool isAction = isupper(nc) || nc == '/' || nc == '*' || nc == '}' ||
                                    source_.substr(look, 6) == "return" ||
                                    source_.substr(look, 5) == "raise" ||
                                    source_.substr(look, 7) == "failwit";
                    if (!isAction && isalpha(nc)) {
                        std::string peek;
                        size_t p2 = look;
                        while (p2 < source_.size() && (isalnum(source_[p2]) || source_[p2]=='_'))
                            peek += source_[p2++];
                        if (!ruleName_.empty() && peek == ruleName_) isAction = true;
                    }
                    if (isAction) break;
                }
            }
            braceDepth++;
            pattern += c; pos_++;

        } else if (c == '}') {
            if (braceDepth > 0) {
                braceDepth--;
                pattern += c; pos_++;
            } else {
                break;
            }

        } else if (c == '\n') {
            lineNum_++; pos_++;
            if (!pattern.empty()) {
                trimRight(pattern);
                if (!pattern.empty()) break;
            }

        } else {
            pattern += c; pos_++;
        }
    }

    trimInPlace(pattern);
    if (pattern.empty()) error("Patrón vacío");
    return pattern;
}

// ════════════════════════════════════════════════════════════
//  readTokenName — lee la acción y extrae el nombre del token
// ════════════════════════════════════════════════════════════
std::string YAlexReader::readTokenName() {
    skipWhitespaceAndComments();
    if (atEnd() || current() != '{') error("Se esperaba '{' para la acción del token");
    pos_++; // consume '{'
    skipWS(); // solo espacios, no newlines

    // ── Acción vacía: { } ────────────────────────────────────────
    if (!atEnd() && current() == '}') {
        pos_++;
        return "__SKIP__";
    }

    // ── Comentario de skip: { (* skip *) } o { /* skip */ } ─────
    if (!atEnd() && current() == '(') {
        if (pos_+1 < source_.size() && source_[pos_+1] == '*') {
            // skip hasta *)
            pos_ += 2;
            while (!atEnd()) {
                if (current() == '*' && pos_+1 < source_.size() && source_[pos_+1] == ')') {
                    pos_ += 2; break;
                }
                if (current() == '\n') lineNum_++;
                pos_++;
            }
            skipWS();
            if (!atEnd() && current() == '}') pos_++;
            return "__SKIP__";
        }
    }
    if (!atEnd() && current() == '/') {
        if (pos_+1 < source_.size() && source_[pos_+1] == '*') {
            pos_ += 2;
            while (!atEnd()) {
                if (current() == '*' && pos_+1 < source_.size() && source_[pos_+1] == '/') {
                    pos_ += 2; break;
                }
                if (current() == '\n') lineNum_++;
                pos_++;
            }
            skipWS();
            if (!atEnd() && current() == '}') pos_++;
            return "__SKIP__";
        }
    }

    // ── Leer todo el contenido de la acción ─────────────────────
    std::string content;
    int depth = 1;
    while (!atEnd() && depth > 0) {
        char c = current();
        if (c == '{')      { depth++; content += c; pos_++; }
        else if (c == '}') {
            depth--;
            if (depth > 0) { content += c; pos_++; }
            else pos_++;
        }
        else if (c == '"') {
            // String literal dentro de la acción
            content += c; pos_++;
            while (!atEnd() && current() != '"') {
                if (current() == '\\') { content += current(); pos_++; }
                if (!atEnd()) { content += current(); pos_++; }
            }
            if (!atEnd()) { content += current(); pos_++; } // cierra "
        }
        else {
            if (c == '\n') lineNum_++;
            content += c; pos_++;
        }
    }

    // ── Parsear el contenido para extraer el nombre del token ────
    return extractTokenName(content);
}

// ════════════════════════════════════════════════════════════
//  extractTokenName — determina el nombre del token desde el
//  contenido de la acción (ya sin las llaves externas)
// ════════════════════════════════════════════════════════════
std::string YAlexReader::extractTokenName(const std::string& raw) {
    // Trim
    std::string s = raw;
    size_t st = s.find_first_not_of(" \t\n\r");
    if (st == std::string::npos) return "__SKIP__";
    s = s.substr(st);
    size_t en = s.find_last_not_of(" \t\n\r;");
    if (en != std::string::npos) s = s.substr(0, en + 1);

    // Vacío
    if (s.empty()) return "__SKIP__";

    // ── "return TOKEN" / "return TOKEN;" ────────────────────────
    if (s.size() > 7 && s.substr(0, 7) == "return ") {
        std::string rest = s.substr(7);
        size_t r = rest.find_first_not_of(" \t");
        if (r != std::string::npos) rest = rest.substr(r);
        // Quitar ';' y cualquier argumento '(...)'
        size_t end = rest.size();
        for (size_t i = 0; i < rest.size(); i++) {
            char c = rest[i];
            if (c == ';' || c == '(' || c == ' ' || c == '\t') { end = i; break; }
        }
        rest = rest.substr(0, end);
        if (!rest.empty()) return rest;
    }

    // ── Primera palabra del contenido ───────────────────────────
    std::string first;
    for (char c : s) {
        if (isalnum(c) || c == '_') first += c;
        else break;
    }

    if (first.empty()) return "__SKIP__";

    // ── Acciones OCamllex a descartar ────────────────────────────
    // "token lexbuf"  → recursión (skip)
    // "failwith ..."  → error (skip)
    // "raise ..."     → excepción (skip)
    // "ignore ..."    → skip
    if (first == "failwith" || first == "raise" ||
        first == "ignore"   || first == "assert") {
        return "__SKIP__";
    }
    // Si es el nombre de la regla misma → recursión → skip
    if (!ruleName_.empty() && first == ruleName_) {
        return "__SKIP__";
    }

    // ── Si hay argumento entre paréntesis: TOKEN(expr) ──────────
    // En OCamllex: { INT (int_of_string ...) } → TOKEN = "INT"
    // Solo tomamos la primera palabra si es todo UPPER_CASE o conocida
    size_t paren = s.find('(');
    if (paren != std::string::npos) {
        std::string candidate = s.substr(0, paren);
        size_t ce = candidate.find_last_not_of(" \t");
        if (ce != std::string::npos) candidate = candidate.substr(0, ce + 1);
        if (!candidate.empty()) return candidate;
    }

    return first;
}

// ════════════════════════════════════════════════════════════
//  normalizeCharClass — convierte clases de caracteres flex
//  [ \t\n\r]  →  [' ' '\t' '\n' '\r']
//  Esto permite usar el formato flex directamente.
// ════════════════════════════════════════════════════════════
// Función libre: normaliza clases de caracteres estilo flex
// SOLO actúa si la clase contiene secuencias \t \n \r sin comillas.
// Si el patrón ya usa 'x' (formato proyecto), se devuelve intacto.
static std::string normalizeCharClassFlex(const std::string& pat) {
    // Detectar si necesita normalización:
    // Busca \t, \n, \r, \s, \d dentro de [...] sin estar entre comillas simples
    auto needsNorm = [&]() -> bool {
        bool inClass = false;
        bool inQuote = false;
        for (size_t i = 0; i < pat.size(); i++) {
            if (!inClass && pat[i] == '[') { inClass = true; continue; }
            if (!inClass) continue;
            if (pat[i] == ']' && !inQuote) { inClass = false; continue; }
            if (pat[i] == '\'' ) { inQuote = !inQuote; continue; }
            if (!inQuote && pat[i] == '\\' && i+1 < pat.size()) {
                char n = pat[i+1];
                if (n == 't' || n == 'n' || n == 'r' || n == 's' || n == 'd' || n == 'w')
                    return true;
            }
        }
        return false;
    };

    if (!needsNorm()) return pat; // ya está bien formateado

    // Aplica normalización: convierte \t→'\t', \n→'\n', \r→'\r', etc.
    std::string result;
    size_t i = 0;
    while (i < pat.size()) {
        if (pat[i] != '[') { result += pat[i++]; continue; }

        result += '['; i++;
        while (i < pat.size() && pat[i] != ']') {
            if (pat[i] == '\'' ) {
                // Ya quoted 'x' → copiar tal cual hasta cierre
                result += pat[i++]; // abre '
                while (i < pat.size() && pat[i] != '\'') result += pat[i++];
                if (i < pat.size()) result += pat[i++]; // cierra '
                continue;
            }
            if (pat[i] == '\\' && i+1 < pat.size()) {
                char next = pat[i+1];
                switch (next) {
                    case 't': result += "'\t'"; break;
                    case 'n': result += "'\n'"; break;
                    case 'r': result += "'\r'"; break;
                    case 's': result += "' ''\t''\n''\r'"; break;
                    case 'd': result += "0-9"; break;
                    case 'w': result += "a-zA-Z0-9_"; break;
                    default:  result += std::string("'") + next + "'"; break;
                }
                i += 2; continue;
            }
            // Espacio literal sin comillas → envolverlo
            if (pat[i] == ' ') { result += "' '"; i++; continue; }
            result += pat[i++];
        }
        if (i < pat.size()) { result += ']'; i++; }
    }
    return result;
}

// ════════════════════════════════════════════════════════════
//  expandBareMacros — expande nombres de macro sin llaves
//  Solo para formato .yal donde los macros se usan sin {braces}
// ════════════════════════════════════════════════════════════
std::string YAlexReader::expandBareMacros(
        const std::string& pattern,
        const std::unordered_map<std::string,std::string>& macros) {

    // Si el patrón ya tiene {macros} → no expandir bare
    bool hasBraces = pattern.find('{') != std::string::npos;

    std::string result;
    size_t i = 0;
    while (i < pattern.size()) {
        char c = pattern[i];

        // String entre comillas dobles → copiar literal
        if (c == '"') {
            result += c; i++;
            while (i < pattern.size() && pattern[i] != '"') {
                if (pattern[i] == '\\') { result += pattern[i++]; }
                if (i < pattern.size()) result += pattern[i++];
            }
            if (i < pattern.size()) { result += pattern[i++]; }
            continue;
        }

        // Clase de caracteres [...] → copiar literal
        if (c == '[') {
            result += c; i++;
            while (i < pattern.size() && pattern[i] != ']') {
                if (pattern[i] == '\\') { result += pattern[i++]; }
                if (i < pattern.size()) result += pattern[i++];
            }
            if (i < pattern.size()) { result += pattern[i++]; }
            continue;
        }

        // Char entre comillas simples 'x' → copiar literal
        if (c == '\'') {
            result += c; i++;
            while (i < pattern.size() && pattern[i] != '\'') {
                if (pattern[i] == '\\') { result += pattern[i++]; }
                if (i < pattern.size()) result += pattern[i++];
            }
            if (i < pattern.size()) { result += pattern[i++]; }
            continue;
        }

        // Ya entre llaves {macro} → copiar sin cambios
        if (c == '{') {
            result += c; i++;
            while (i < pattern.size() && pattern[i] != '}') { result += pattern[i++]; }
            if (i < pattern.size()) { result += pattern[i++]; }
            continue;
        }

        // Identificador → posible macro bare (solo si el patrón no tiene llaves)
        if (!hasBraces && (isalpha(c) || c == '_')) {
            std::string word;
            while (i < pattern.size() && (isalnum(pattern[i]) || pattern[i] == '_'))
                word += pattern[i++];
            if (macros.count(word)) result += "{" + word + "}";
            else result += word;
            continue;
        }

        // Espacio en formato .yal es separador de concatenación → omitir
        if (!hasBraces && (c == ' ' || c == '\t')) { i++; continue; }

        result += c; i++;
    }
    return result;
}

// ════════════════════════════════════════════════════════════
//  skipWhitespaceAndComments — salta ws y TODOS los estilos de comentario
// ════════════════════════════════════════════════════════════
void YAlexReader::skipWhitespaceAndComments() {
    while (!atEnd()) {
        char c = current();

        if (c == ' ' || c == '\t' || c == '\r') { pos_++; continue; }
        if (c == '\n') { pos_++; lineNum_++; continue; }

        // (* ... *) — OCamllex / yalex
        if (c == '(' && pos_+1 < source_.size() && source_[pos_+1] == '*') {
            pos_ += 2;
            while (!atEnd()) {
                if (current() == '*' && pos_+1 < source_.size() && source_[pos_+1] == ')') {
                    pos_ += 2; break;
                }
                if (current() == '\n') lineNum_++;
                pos_++;
            }
            continue;
        }

        // // ... \n — C++ / yacc
        if (c == '/' && pos_+1 < source_.size() && source_[pos_+1] == '/') {
            pos_ += 2;
            while (!atEnd() && current() != '\n') pos_++;
            continue;
        }

        // /* ... */ — C / flex
        if (c == '/' && pos_+1 < source_.size() && source_[pos_+1] == '*') {
            pos_ += 2;
            while (!atEnd()) {
                if (current() == '*' && pos_+1 < source_.size() && source_[pos_+1] == '/') {
                    pos_ += 2; break;
                }
                if (current() == '\n') lineNum_++;
                pos_++;
            }
            continue;
        }

        // # ... \n — shell / Python
        if (c == '#') {
            while (!atEnd() && current() != '\n') pos_++;
            continue;
        }

        break; // caracter significativo
    }
}

// ── Salta solo espacios horizontales (no newlines) ───────────
void YAlexReader::skipWS() {
    while (!atEnd() && (current() == ' ' || current() == '\t' || current() == '\r'))
        pos_++;
}

// ── Salta hasta el final de la línea actual (inclusive) ──────
void YAlexReader::skipToEndOfLine() {
    while (!atEnd() && current() != '\n') pos_++;
    if (!atEnd()) { pos_++; lineNum_++; }
}

// ════════════════════════════════════════════════════════════
//  Utilidades básicas
// ════════════════════════════════════════════════════════════
std::string YAlexReader::readWord() {
    std::string w;
    while (!atEnd() && (isalnum(current()) || current() == '_'))
        w += current(), pos_++;
    return w;
}

std::string YAlexReader::readUntilEndOfLine() {
    std::string r;
    while (!atEnd() && current() != '\n' && current() != '\r')
        r += current(), pos_++;
    return r;
}

void YAlexReader::trimInPlace(std::string& s) {
    size_t st = s.find_first_not_of(" \t\r\n");
    if (st == std::string::npos) { s.clear(); return; }
    s = s.substr(st);
    size_t en = s.find_last_not_of(" \t\r\n");
    if (en != std::string::npos) s = s.substr(0, en + 1);
}

void YAlexReader::trimRight(std::string& s) {
    size_t en = s.find_last_not_of(" \t\r");
    if (en != std::string::npos) s = s.substr(0, en + 1);
    else s.clear();
}

void YAlexReader::expect(const std::string& word) {
    skipWhitespaceAndComments();
    std::string found = readWord();
    if (found != word)
        error("Se esperaba '" + word + "', encontrado: '" + found + "'");
}

void YAlexReader::error(const std::string& msg) {
    throw std::runtime_error(sourceName_ + ":" + std::to_string(lineNum_) + ": " + msg);
}

} // namespace yalex
