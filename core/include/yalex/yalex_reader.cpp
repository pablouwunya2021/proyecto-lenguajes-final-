// ============================================================
//  yalex_reader.cpp
//  Implementación del parser de archivos .yalex
// ============================================================

#include "yalex/yalex_reader.hpp"
#include <fstream>
#include <sstream>
#include <cctype>
#include <iostream>

namespace yalex {

// ════════════════════════════════════════════════════════════
//  readFile() — carga el archivo del disco y llama readString
// ════════════════════════════════════════════════════════════
YAlexSpec YAlexReader::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open())
        throw std::runtime_error("No se pudo abrir el archivo: " + filepath);

    // Leemos todo el contenido de una vez
    std::ostringstream ss;
    ss << file.rdbuf();
    return readString(ss.str(), filepath);
}

// ════════════════════════════════════════════════════════════
//  readString() — inicializa el estado y lanza el parseo
// ════════════════════════════════════════════════════════════
YAlexSpec YAlexReader::readString(const std::string& content,
                                   const std::string& sourceName) {
    // Inicializamos el estado del parser
    source_     = content;
    pos_        = 0;
    lineNum_    = 1;
    sourceName_ = sourceName;

    YAlexSpec spec;
    spec.sourceFile = sourceName;

    skipWhitespaceAndComments();

    // ── Estructura del archivo .yalex ─────────────────────────
    // Primero viene el bloque de macros (opcional): { ... }
    // Luego viene la sección de reglas: rule tokens = ...
    if (!atEnd() && current() == '{') {
        parseMacroBlock(spec);
        skipWhitespaceAndComments();
    }

    // Esperamos la palabra "rule"
    if (!atEnd()) {
        expect("rule");
        skipWhitespaceAndComments();

        // Nombre de la sección (normalmente "tokens", lo ignoramos)
        readWord();
        skipWhitespaceAndComments();

        // Esperamos el '='
        if (atEnd() || current() != '=')
            error("Se esperaba '=' después del nombre de la regla");
        pos_++;

        parseRuleSection(spec);
    }

    if (spec.rules.empty())
        error("El archivo .yalex no contiene ninguna regla");

    return spec;
}

// ════════════════════════════════════════════════════════════
//  parseMacroBlock() — parsea el bloque { let x = regex ... }
// ════════════════════════════════════════════════════════════
void YAlexReader::parseMacroBlock(YAlexSpec& spec) {
    // Consumimos el '{'
    pos_++;
    skipWhitespaceAndComments();

    // Leemos declaraciones de macro hasta encontrar el '}'
    while (!atEnd() && current() != '}') {
        // Cada declaración empieza con "let"
        std::string word = readWord();
        if (word == "let") {
            parseMacroDecl(spec);
        } else {
            error("Se esperaba 'let' dentro del bloque de macros, encontrado: " + word);
        }
        skipWhitespaceAndComments();
    }

    if (atEnd())
        error("Bloque de macros sin cerrar '}'");

    pos_++; // consumimos el '}'
}

// ════════════════════════════════════════════════════════════
//  parseMacroDecl() — parsea: let nombre = expresion_regular
// ════════════════════════════════════════════════════════════
void YAlexReader::parseMacroDecl(YAlexSpec& spec) {
    skipWhitespaceAndComments();

    // Nombre de la macro
    std::string name = readWord();
    if (name.empty())
        error("Se esperaba nombre de macro después de 'let'");

    skipWhitespaceAndComments();

    // '='
    if (atEnd() || current() != '=')
        error("Se esperaba '=' en declaración de macro '" + name + "'");
    pos_++;

    skipWhitespaceAndComments();

    // La expresión regular va hasta el final de la línea
    // (o hasta el próximo 'let' o '}')
    std::string pattern = readUntilEndOfLine();

    // Limpiamos espacios al inicio y al final
    size_t start = pattern.find_first_not_of(" \t");
    size_t end   = pattern.find_last_not_of(" \t\r");
    if (start == std::string::npos)
        error("Macro '" + name + "' tiene expresión vacía");

    pattern = pattern.substr(start, end - start + 1);
    spec.macros[name] = pattern;
}

// ════════════════════════════════════════════════════════════
//  parseRuleSection() — parsea las reglas léxicas
//
//  Formato:
//    regex1 { TOKEN1 }
//  | regex2 { TOKEN2 }
//  | regex3 { TOKEN3 }
//
//  La primera regla NO tiene '|' al inicio.
//  Las demás empiezan con '|'.
// ════════════════════════════════════════════════════════════
void YAlexReader::parseRuleSection(YAlexSpec& spec) {
    skipWhitespaceAndComments();
    int priority = 0;

    // Primera regla (sin '|' al inicio)
    if (!atEnd() && current() != '|') {
        spec.rules.push_back(parseOneRule(priority++));
    }

    skipWhitespaceAndComments();

    // Reglas adicionales (cada una empieza con '|')
    while (!atEnd() && current() == '|') {
        pos_++; // consumimos el '|'
        skipWhitespaceAndComments();
        spec.rules.push_back(parseOneRule(priority++));
        skipWhitespaceAndComments();
    }
}

// ════════════════════════════════════════════════════════════
//  parseOneRule() — parsea: regex { TOKEN }
// ════════════════════════════════════════════════════════════
LexRule YAlexReader::parseOneRule(int priority) {
    LexRule rule;
    rule.priority  = priority;
    rule.pattern   = readPattern();
    rule.tokenName = readTokenName();
    return rule;
}

// ════════════════════════════════════════════════════════════
//  readPattern() — lee la regex hasta encontrar el '{'
//  Tiene cuidado con los '{'  que están dentro de nombres
//  de macros: {letter} no es el inicio del nombre del token.
// ════════════════════════════════════════════════════════════
std::string YAlexReader::readPattern() {
    std::string pattern;
    int braceDepth = 0; // profundidad de llaves anidadas

    while (!atEnd()) {
        char c = current();

        if (c == '{') {
            // Puede ser inicio de macro {nombre} o del token {TOKEN}
            // Para distinguirlos: miramos si después hay una letra
            // Si es macro, el contenido es un identificador
            // Si es acción, generalmente el contenido es un token en mayúsculas

            // Estrategia simple: contamos las llaves
            // Si encontramos '{' seguido de letras y luego '}' con espacios
            // alrededor, asumimos que es el nombre del token
            // Para ser más robustos, verificamos si braceDepth == 0

            if (braceDepth == 0 && !pattern.empty()) {
                // Miramos adelante: ¿es { TOKEN } o { macro }?
                // Si el patrón ya tiene contenido, este '{' cierra la regla
                // Heurística: si el caracter siguiente es mayúscula o espacio+mayúscula
                size_t lookahead = pos_ + 1;
                while (lookahead < source_.size() && source_[lookahead] == ' ')
                    lookahead++;

                if (lookahead < source_.size() && isupper(source_[lookahead])) {
                    // Es el { TOKEN } → paramos aquí
                    break;
                }
            }

            braceDepth++;
            pattern += c;
            pos_++;
        } else if (c == '}') {
            if (braceDepth > 0) {
                braceDepth--;
                pattern += c;
                pos_++;
            } else {
                // '}' inesperado con braceDepth == 0
                break;
            }
        } else if (c == '\n') {
            lineNum_++;
            pos_++;
            // Las reglas pueden continuar en la siguiente línea
            // si la siguiente línea no empieza con '|' ni '}'
            // Por simplicidad, tratamos el newline como separador
            // y paramos si el patrón ya tiene contenido
            if (!pattern.empty()) {
                // Limpiamos trailing spaces
                size_t last = pattern.find_last_not_of(" \t");
                if (last != std::string::npos)
                    pattern = pattern.substr(0, last + 1);
                break;
            }
        } else {
            pattern += c;
            pos_++;
        }
    }

    // Limpiamos espacios del patrón
    size_t start = pattern.find_first_not_of(" \t");
    size_t end   = pattern.find_last_not_of(" \t");
    if (start == std::string::npos)
        error("Regla con patrón vacío");

    return pattern.substr(start, end - start + 1);
}

// ════════════════════════════════════════════════════════════
//  readTokenName() — lee el nombre del token en { TOKEN }
// ════════════════════════════════════════════════════════════
std::string YAlexReader::readTokenName() {
    skipWhitespaceAndComments();

    if (atEnd() || current() != '{')
        error("Se esperaba '{' para el nombre del token");
    pos_++; // consumimos '{'

    skipWhitespaceAndComments();

    // Leemos el nombre del token (letras, dígitos, _)
    std::string name;
    while (!atEnd() && (isalnum(current()) || current() == '_')) {
        name += current();
        pos_++;
    }

    if (name.empty())
        error("Nombre de token vacío");

    skipWhitespaceAndComments();

    if (atEnd() || current() != '}')
        error("Se esperaba '}' después del nombre del token '" + name + "'");
    pos_++; // consumimos '}'

    return name;
}

// ════════════════════════════════════════════════════════════
//  Helpers
// ════════════════════════════════════════════════════════════

void YAlexReader::skipWhitespaceAndComments() {
    while (!atEnd()) {
        char c = current();

        if (c == ' ' || c == '\t' || c == '\r') {
            pos_++;
        } else if (c == '\n') {
            pos_++;
            lineNum_++;
        } else if (c == '(' && pos_ + 1 < source_.size() && source_[pos_+1] == '*') {
            // Comentario (* ... *) estilo OCaml (el formato de YALex real)
            pos_ += 2;
            while (!atEnd()) {
                if (current() == '*' && pos_+1 < source_.size() && source_[pos_+1] == ')') {
                    pos_ += 2;
                    break;
                }
                if (current() == '\n') lineNum_++;
                pos_++;
            }
        } else if (c == '#') {
            // Comentario de línea estilo Python/shell
            while (!atEnd() && current() != '\n')
                pos_++;
        } else {
            break;
        }
    }
}

std::string YAlexReader::readWord() {
    std::string word;
    while (!atEnd() && (isalnum(current()) || current() == '_')) {
        word += current();
        pos_++;
    }
    return word;
}

std::string YAlexReader::readUntilEndOfLine() {
    std::string result;
    while (!atEnd() && current() != '\n' && current() != '\r') {
        result += current();
        pos_++;
    }
    return result;
}

void YAlexReader::expect(const std::string& word) {
    skipWhitespaceAndComments();
    std::string found = readWord();
    if (found != word)
        error("Se esperaba '" + word + "', encontrado: '" + found + "'");
}

void YAlexReader::error(const std::string& msg) {
    throw std::runtime_error(
        sourceName_ + ":" + std::to_string(lineNum_) + ": Error: " + msg
    );
}

} // namespace yalex