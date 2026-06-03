// ============================================================
//  grammar.cpp — Carga de gramática + FIRST / FOLLOW
// ============================================================
#include "yapar/grammar.hpp"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <stdexcept>
#include <cctype>

namespace yapar {

// ════════════════════════════════════════════════════════════
//  loadFromFile / loadFromString
// ════════════════════════════════════════════════════════════
void Grammar::loadFromFile(const std::string& filepath) {
    std::ifstream f(filepath);
    if (!f.is_open()) throw std::runtime_error("No se pudo abrir: " + filepath);
    std::ostringstream ss; ss << f.rdbuf();
    loadFromString(ss.str());
}

void Grammar::loadFromString(const std::string& content) {
    productions_.clear();
    terminals_.clear();
    nonTerminals_.clear();
    first_.clear();
    follow_.clear();
    precMap_.clear();
    prodPrecToken_.clear();
    sawSeparator_ = false;
    parse(content);
    validate();
    computeFirstAndFollow();
}

// ════════════════════════════════════════════════════════════
//  validate() — verifica que la gramática leída sea coherente
//  Lanza excepciones claras en lugar de fallar de forma críptica
//  más adelante (al construir el autómata o al parsear).
// ════════════════════════════════════════════════════════════
void Grammar::validate() const {
    // 1. Debe existir el separador %% (si no, no se leyó ninguna regla)
    if (!sawSeparator_) {
        throw std::runtime_error(
            "La gramatica no contiene el separador '%%' que divide las "
            "declaraciones (%token, %start, ...) de las reglas de produccion. "
            "Agrega una linea con '%%' antes de las producciones.");
    }

    // 2. Debe haber al menos una produccion
    if (productions_.empty()) {
        throw std::runtime_error(
            "La gramatica no tiene producciones despues del '%%'. "
            "Define al menos una regla con la forma 'no_terminal : simbolos'.");
    }

    // 3. El simbolo inicial debe tener al menos una produccion
    if (startSymbol_.name.empty()) {
        throw std::runtime_error("No se pudo determinar el simbolo inicial de la gramatica.");
    }
    if (getProductionsFor(startSymbol_.name).empty()) {
        throw std::runtime_error(
            "El simbolo inicial '" + startSymbol_.name +
            "' no tiene ninguna produccion definida.");
    }

    // 4. Todo no-terminal usado debe tener al menos una produccion
    std::vector<std::string> undefined;
    for (const auto& nt : nonTerminals_) {
        if (getProductionsFor(nt.name).empty()) {
            // Evitar duplicados
            if (std::find(undefined.begin(), undefined.end(), nt.name) == undefined.end())
                undefined.push_back(nt.name);
        }
    }
    if (!undefined.empty()) {
        std::string list;
        for (size_t i = 0; i < undefined.size(); i++) {
            if (i) list += ", ";
            list += "'" + undefined[i] + "'";
        }
        throw std::runtime_error(
            "Los siguientes no-terminales se usan en el lado derecho de alguna "
            "produccion pero nunca se definen: " + list +
            ". Revisa que no haya errores de tipografia o reglas faltantes "
            "(recuerda que los terminales se escriben en MAYUSCULAS y se "
            "declaran con %token).");
    }
}

// ════════════════════════════════════════════════════════════
//  parse() — lee el archivo .yapar
//
//  Formato soportado:
//    %token ID INT PLUS ...       ← declara terminales
//    %%
//    program : stmt_list ;
//    stmt_list : stmt_list stmt
//              | stmt
//              ;
//    ...
// ════════════════════════════════════════════════════════════
void Grammar::parse(const std::string& rawContent) {
    // ── Quitar BOM UTF-8 ─────────────────────────────────────────
    std::string content = rawContent;
    if (content.size() >= 3 &&
        (unsigned char)content[0] == 0xEF &&
        (unsigned char)content[1] == 0xBB &&
        (unsigned char)content[2] == 0xBF) content = content.substr(3);

    // ── Quitar bloques %{ ... %} ─────────────────────────────────
    {
        std::string clean;
        size_t i = 0;
        while (i < content.size()) {
            if (i+1 < content.size() && content[i]=='%' && content[i+1]=='{') {
                i += 2;
                while (i < content.size()) {
                    if (i+1 < content.size() && content[i]=='%' && content[i+1]=='}') { i+=2; break; }
                    if (content[i]=='\n') clean += '\n'; // preservar líneas para números
                    i++;
                }
            } else {
                clean += content[i++];
            }
        }
        content = clean;
    }

    // ── Quitar comentarios /* ... */ (multilínea) ────────────────
    {
        std::string clean;
        size_t i = 0;
        while (i < content.size()) {
            if (i+1 < content.size() && content[i]=='/' && content[i+1]=='*') {
                i += 2;
                while (i < content.size()) {
                    if (i+1 < content.size() && content[i]=='*' && content[i+1]=='/') { i+=2; break; }
                    if (content[i]=='\n') clean += '\n';
                    i++;
                }
            } else {
                clean += content[i++];
            }
        }
        content = clean;
    }

    std::istringstream stream(content);
    std::string line;
    bool inRules = false;
    std::string currentLHS;
    int prodId    = 0;
    int precLevel = 0; // sube con cada directiva %left/%right/%nonassoc
    int lineNum   = 0;

    while (std::getline(stream, line)) {
        lineNum++;
        // Quitar comentarios // y (* ... *) en la misma línea
        {
            // //
            size_t c2 = line.find("//");
            if (c2 != std::string::npos) line = line.substr(0, c2);
            // (* ... *) simple en una línea
            size_t oc = line.find("(*");
            size_t cc = line.find("*)");
            if (oc != std::string::npos && cc != std::string::npos && cc > oc)
                line = line.substr(0, oc) + line.substr(cc + 2);
        }

        // Trim
        size_t s = line.find_first_not_of(" \t\r");
        if (s == std::string::npos) continue;
        line = line.substr(s);
        size_t e = line.find_last_not_of(" \t\r");
        if (e != std::string::npos) line = line.substr(0, e+1);
        if (line.empty()) continue;

        // Separador entre tokens y reglas
        if (line == "%%" || line == "%%\r") { inRules = true; sawSeparator_ = true; continue; }

        if (!inRules) {
            // Sección de declaraciones: %token, %start, %left, %right, %nonassoc
            auto startsWith = [&](const std::string& prefix) {
                return line.size() >= prefix.size() &&
                       line.substr(0, prefix.size()) == prefix &&
                       (line.size() == prefix.size() || line[prefix.size()] == ' ' || line[prefix.size()] == '\t');
            };

            if (startsWith("%token")) {
                std::istringstream ts(line.substr(6));
                std::string tok;
                while (ts >> tok) {
                    // ignorar comentarios inline después del token
                    if (tok == "//" || tok == "/*") break;
                    addSymbol(Symbol::terminal(tok));
                }
            } else if (startsWith("%start")) {
                std::istringstream ts(line.substr(6));
                std::string name; ts >> name;
                if (!name.empty()) startSymbol_ = Symbol::nonTerminal(name);
            } else if (startsWith("%left") || startsWith("%right") || startsWith("%nonassoc")) {
                // Determinar asociatividad
                Associativity assoc;
                size_t skip;
                if (startsWith("%left"))     { assoc = Associativity::LEFT;    skip = 5; }
                else if (startsWith("%right")){ assoc = Associativity::RIGHT;   skip = 6; }
                else                          { assoc = Associativity::NONASSOC; skip = 9; }

                precLevel++; // nuevo nivel de precedencia
                std::istringstream ts(line.substr(skip));
                std::string tok;
                while (ts >> tok) {
                    addSymbol(Symbol::terminal(tok));
                    precMap_[tok] = {precLevel, assoc};
                }
            }
        } else {
            // Sección de reglas gramaticales
            // Formato: LHS : sym1 sym2 ... | sym3 ... ;
            if (line == ";") continue;  // fin de regla

            // ¿Empieza con un no-terminal seguido de ':'?
            size_t colon = line.find(':');
            if (colon != std::string::npos && !line.empty() && line[0] != '|') {
                currentLHS = line.substr(0, colon);
                // Quitar espacios del LHS
                size_t lEnd = currentLHS.find_last_not_of(" \t");
                if (lEnd != std::string::npos) currentLHS = currentLHS.substr(0, lEnd+1);
                addSymbol(Symbol::nonTerminal(currentLHS));
                if (startSymbol_.name.empty()) startSymbol_ = Symbol::nonTerminal(currentLHS);
                line = line.substr(colon + 1);
            }

            // Procesar alternativas separadas por '|'
            // Cada alternativa puede estar en la misma línea o en líneas con '|'
            if (!line.empty() && line[0] == '|') line = line.substr(1);

            // Quitar el ';' al final si existe
            if (!line.empty() && line.back() == ';') line.pop_back();

            // Separar '|' y ';' aunque estén pegados a un símbolo.
            // Ej: "VERB_PAINT| VERB_SEW" debe leerse como dos símbolos
            // y un separador, no como el símbolo "VERB_PAINT|".
            {
                std::string padded;
                for (char ch : line) {
                    if (ch == '|' || ch == ';') { padded += ' '; padded += ch; padded += ' '; }
                    else padded += ch;
                }
                line = padded;
            }

            // Dividir por '|' en la misma línea
            std::istringstream altStream(line);
            std::string token;
            Production prod;
            prod.id         = prodId++;
            prod.lhs        = Symbol::nonTerminal(currentLHS);
            prod.sourceLine = lineNum;

            // Leemos símbolo por símbolo
            // Si encontramos '|', iniciamos nueva producción
            bool nextIsPrec = false; // flag: el siguiente token es el argumento de %prec
            while (altStream >> token) {
                if (token == "|") {
                    productions_.push_back(prod);
                    prod = Production();
                    prod.id         = prodId++;
                    prod.lhs        = Symbol::nonTerminal(currentLHS);
                    prod.sourceLine = lineNum;
                    nextIsPrec = false;
                } else if (token == ";" || token == "/*" || token == "*/") {
                    continue;
                } else if (token == "%prec") {
                    // El siguiente token será el nombre del terminal cuya precedencia usar
                    nextIsPrec = true;
                } else if (nextIsPrec) {
                    // Guardamos el override de precedencia para esta producción
                    prodPrecToken_[prod.id] = token;
                    nextIsPrec = false;
                } else {
                    bool isT = std::all_of(token.begin(), token.end(),
                                           [](char c){ return isupper(c) || c=='_'; });
                    for (const auto& t : terminals_)
                        if (t.name == token) { isT = true; break; }

                    Symbol sym = isT ? Symbol::terminal(token)
                                     : Symbol::nonTerminal(token);
                    addSymbol(sym);
                    prod.rhs.push_back(sym);
                }
            }
            if (!prod.lhs.name.empty())
                productions_.push_back(prod);
        }
    }

    // ── Aumentar la gramática: S' → S ──────────────────────────
    // Imprescindible para construir tablas SLR/LALR correctas.
    // Sin esta producción, el "reduce" del símbolo inicial se
    // confunde con ACCEPT y los conflictos shift/reduce de
    // gramáticas con símbolo inicial recursivo desaparecen
    // (la tabla deja de ser certera).
    if (!startSymbol_.name.empty() && !productions_.empty()) {
        std::string augName = startSymbol_.name + "'";
        // Evitar colisión (muy improbable) con un no-terminal existente
        bool collide = true;
        while (collide) {
            collide = false;
            for (const auto& nt : nonTerminals_)
                if (nt.name == augName) { collide = true; augName += "'"; break; }
        }

        Production aug;
        aug.lhs        = Symbol::nonTerminal(augName);
        aug.rhs        = { startSymbol_ };
        aug.sourceLine = 0;

        // Insertar al frente (id 0) y renumerar el resto
        productions_.insert(productions_.begin(), aug);
        for (size_t i = 0; i < productions_.size(); i++)
            productions_[i].id = (int)i;

        addSymbol(Symbol::nonTerminal(augName));
        startSymbol_ = Symbol::nonTerminal(augName);
    }

    // Agregar símbolo $ como terminal especial
    addSymbol(Symbol::terminal(END_MARKER));
}

void Grammar::addSymbol(const Symbol& s) {
    if (s.isTerminal) {
        for (const auto& t : terminals_) if (t.name == s.name) return;
        terminals_.push_back(s);
    } else {
        for (const auto& n : nonTerminals_) if (n.name == s.name) return;
        nonTerminals_.push_back(s);
    }
}

std::vector<int> Grammar::getProductionsFor(const std::string& name) const {
    std::vector<int> result;
    for (const auto& p : productions_)
        if (p.lhs.name == name) result.push_back(p.id);
    return result;
}

// ════════════════════════════════════════════════════════════
//  computeFirstAndFollow — algoritmo iterativo de punto fijo
// ════════════════════════════════════════════════════════════
void Grammar::computeFirstAndFollow() {
    computeFirst();
    computeFollow();
}

// ── FIRST ─────────────────────────────────────────────────────
// FIRST(a) para terminal a = {a}
// FIRST(A) para no-terminal A:
//   Para cada producción A → X1 X2 ... Xn:
//     Agrega FIRST(X1) - {ε}
//     Si ε ∈ FIRST(X1): agrega FIRST(X2) - {ε}
//     ... y así sucesivamente
//     Si ε ∈ FIRST(Xi) para todo i: agrega ε
void Grammar::computeFirst() {
    // Inicializar: FIRST(a) = {a} para terminales
    for (const auto& t : terminals_)
        first_[t.name].insert(t.name);

    // FIRST(ε) = {ε}
    first_[EPSILON].insert(EPSILON);

    // Iteramos hasta que no haya cambios (punto fijo)
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& prod : productions_) {
            std::string A = prod.lhs.name;
            size_t beforeSize = first_[A].size();

            if (prod.isEpsilon()) {
                // A → ε
                changed |= first_[A].insert(EPSILON).second;
            } else {
                // A → X1 X2 ... Xn
                bool allHaveEpsilon = true;
                for (const auto& Xi : prod.rhs) {
                    // Agregar FIRST(Xi) - {ε} a FIRST(A)
                    for (const auto& f : first_[Xi.name]) {
                        if (f != EPSILON) changed |= first_[A].insert(f).second;
                    }
                    // Si Xi no puede derivar ε, paramos
                    if (!first_[Xi.name].count(EPSILON)) {
                        allHaveEpsilon = false;
                        break;
                    }
                }
                // Si todos los Xi pueden derivar ε, ε ∈ FIRST(A)
                if (allHaveEpsilon)
                    changed |= first_[A].insert(EPSILON).second;
            }
        }
    }
}

// ── FOLLOW ────────────────────────────────────────────────────
// FOLLOW(S) incluye $ (símbolo inicial)
// Para A → α B β:
//   Agrega FIRST(β) - {ε} a FOLLOW(B)
//   Si ε ∈ FIRST(β): agrega FOLLOW(A) a FOLLOW(B)
// Para A → α B:
//   Agrega FOLLOW(A) a FOLLOW(B)
void Grammar::computeFollow() {
    // FOLLOW del símbolo inicial incluye $
    follow_[startSymbol_.name].insert(END_MARKER);

    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& prod : productions_) {
            std::string A = prod.lhs.name;
            for (size_t i = 0; i < prod.rhs.size(); i++) {
                const Symbol& B = prod.rhs[i];
                if (B.isTerminal) continue; // FOLLOW solo para no-terminales

                // β = lo que sigue a B en la producción
                std::vector<Symbol> beta(prod.rhs.begin() + i + 1, prod.rhs.end());

                // FIRST(β)
                auto firstBeta = firstOfSequence(beta);

                // Agregar FIRST(β) - {ε} a FOLLOW(B)
                for (const auto& f : firstBeta) {
                    if (f != EPSILON)
                        changed |= follow_[B.name].insert(f).second;
                }

                // Si ε ∈ FIRST(β) (o β está vacío), agregar FOLLOW(A) a FOLLOW(B)
                if (firstBeta.count(EPSILON) || beta.empty()) {
                    for (const auto& f : follow_[A])
                        changed |= follow_[B.name].insert(f).second;
                }
            }
        }
    }
}

std::set<std::string> Grammar::firstOfSequence(const std::vector<Symbol>& seq) const {
    std::set<std::string> result;
    if (seq.empty()) { result.insert(EPSILON); return result; }

    bool allHaveEpsilon = true;
    for (const auto& sym : seq) {
        auto it = first_.find(sym.name);
        if (it != first_.end()) {
            for (const auto& f : it->second)
                if (f != EPSILON) result.insert(f);
            if (!it->second.count(EPSILON)) { allHaveEpsilon = false; break; }
        } else {
            allHaveEpsilon = false; break;
        }
    }
    if (allHaveEpsilon) result.insert(EPSILON);
    return result;
}

const std::set<std::string>& Grammar::getFirst(const std::string& symbol) const {
    static std::set<std::string> empty;
    auto it = first_.find(symbol);
    return it != first_.end() ? it->second : empty;
}

const std::set<std::string>& Grammar::getFollow(const std::string& symbol) const {
    static std::set<std::string> empty;
    auto it = follow_.find(symbol);
    return it != follow_.end() ? it->second : empty;
}

// ════════════════════════════════════════════════════════════
//  toJSON / print
// ════════════════════════════════════════════════════════════
// ════════════════════════════════════════════════════════════
//  getPrecedence / getProductionPrec
// ════════════════════════════════════════════════════════════
const PrecInfo* Grammar::getPrecedence(const std::string& token) const {
    auto it = precMap_.find(token);
    if (it == precMap_.end()) return nullptr;
    return &it->second;
}

const PrecInfo* Grammar::getProductionPrec(int prodId) const {
    if (prodId < 0 || prodId >= (int)productions_.size()) return nullptr;
    const Production& p = productions_[prodId];

    // ¿Hay override con %prec?
    auto ov = prodPrecToken_.find(prodId);
    if (ov != prodPrecToken_.end()) return getPrecedence(ov->second);

    // Si no, usamos el terminal más a la derecha del RHS
    for (int i = (int)p.rhs.size() - 1; i >= 0; i--) {
        if (p.rhs[i].isTerminal) return getPrecedence(p.rhs[i].name);
    }
    return nullptr;
}

std::string Grammar::toJSON() const {
    std::ostringstream j;
    j << "{\n  \"productions\": [\n";
    for (size_t i = 0; i < productions_.size(); i++) {
        const auto& p = productions_[i];
        j << "    {\"id\":" << p.id << ",\"lhs\":\"" << p.lhs.name << "\",\"rhs\":[";
        for (size_t k = 0; k < p.rhs.size(); k++) {
            if (k) j << ",";
            j << "\"" << p.rhs[k].name << "\"";
        }
        j << "]}";
        if (i+1 < productions_.size()) j << ",";
        j << "\n";
    }
    j << "  ],\n  \"start\": \"" << startSymbol_.name << "\"";

    // Incluir FIRST de no-terminales
    j << ",\n  \"first\": {";
    {
        bool f1 = true;
        for (const auto& nt : nonTerminals_) {
            auto it = first_.find(nt.name);
            if (it == first_.end()) continue;
            if (!f1) j << ","; f1 = false;
            j << "\n    \"" << nt.name << "\": [";
            bool f2 = true;
            for (const auto& s : it->second) {
                if (!f2) j << ","; f2 = false;
                j << "\"" << s << "\"";
            }
            j << "]";
        }
    }
    j << "\n  }";

    // Incluir FOLLOW de no-terminales
    j << ",\n  \"follow\": {";
    {
        bool f1 = true;
        for (const auto& nt : nonTerminals_) {
            auto it = follow_.find(nt.name);
            if (it == follow_.end()) continue;
            if (!f1) j << ","; f1 = false;
            j << "\n    \"" << nt.name << "\": [";
            bool f2 = true;
            for (const auto& s : it->second) {
                if (!f2) j << ","; f2 = false;
                j << "\"" << s << "\"";
            }
            j << "]";
        }
    }
    j << "\n  }";

    // Incluir tabla de precedencias si la hay
    if (!precMap_.empty()) {
        j << ",\n  \"precedence\": [\n";
        bool first = true;
        for (const auto& [tok, info] : precMap_) {
            if (!first) j << ",\n"; first = false;
            std::string assocStr = (info.assoc == Associativity::LEFT)    ? "left"
                                 : (info.assoc == Associativity::RIGHT)   ? "right"
                                 :                                           "nonassoc";
            j << "    {\"token\":\"" << tok << "\",\"level\":" << info.level
              << ",\"assoc\":\"" << assocStr << "\"}";
        }
        j << "\n  ]";
    }
    j << "\n}";
    return j.str();
}

void Grammar::print() const {
    std::cout << "=== Gramática ===\nInicio: " << startSymbol_.name << "\n\n";
    for (const auto& p : productions_) {
        std::cout << p.id << ". " << p.lhs.name << " → ";
        if (p.isEpsilon()) std::cout << "ε";
        else for (const auto& s : p.rhs) std::cout << s.name << " ";
        std::cout << "\n";
    }
    std::cout << "\n--- FIRST ---\n";
    for (const auto& [sym, first] : first_) {
        std::cout << "FIRST(" << sym << ") = { ";
        for (const auto& f : first) std::cout << f << " ";
        std::cout << "}\n";
    }
    std::cout << "\n--- FOLLOW ---\n";
    for (const auto& [sym, follow] : follow_) {
        std::cout << "FOLLOW(" << sym << ") = { ";
        for (const auto& f : follow) std::cout << f << " ";
        std::cout << "}\n";
    }
}

} // namespace yapar
