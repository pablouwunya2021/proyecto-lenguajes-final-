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
    parse(content);
    computeFirstAndFollow();
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
void Grammar::parse(const std::string& content) {
    std::istringstream stream(content);
    std::string line;
    bool inRules = false;
    std::string currentLHS;
    int prodId = 0;

    while (std::getline(stream, line)) {
        // Quitar comentarios C-style: /* ... */ y //
        size_t comment = line.find("//");
        if (comment != std::string::npos) line = line.substr(0, comment);

        // Trim
        size_t s = line.find_first_not_of(" \t\r");
        if (s == std::string::npos) continue;
        line = line.substr(s);
        size_t e = line.find_last_not_of(" \t\r");
        if (e != std::string::npos) line = line.substr(0, e+1);
        if (line.empty()) continue;

        // Separador entre tokens y reglas
        if (line == "%%") { inRules = true; continue; }

        if (!inRules) {
            // Sección de declaraciones: %token, %start, etc.
            if (line.substr(0, 6) == "%token") {
                std::istringstream ts(line.substr(6));
                std::string tok;
                while (ts >> tok) addSymbol(Symbol::terminal(tok));
            } else if (line.substr(0, 6) == "%start") {
                std::istringstream ts(line.substr(6));
                std::string name; ts >> name;
                startSymbol_ = Symbol::nonTerminal(name);
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

            // Dividir por '|' en la misma línea
            std::istringstream altStream(line);
            std::string token;
            Production prod;
            prod.id  = prodId++;
            prod.lhs = Symbol::nonTerminal(currentLHS);

            // Leemos símbolo por símbolo
            // Si encontramos '|', iniciamos nueva producción
            while (altStream >> token) {
                if (token == "|") {
                    // Guardamos la producción actual y empezamos una nueva
                    productions_.push_back(prod);
                    prod = Production();
                    prod.id  = prodId++;
                    prod.lhs = Symbol::nonTerminal(currentLHS);
                } else if (token == ";" || token == "/*" || token == "*/") {
                    continue;
                } else {
                    // ¿Es terminal o no-terminal?
                    // Convención: terminales en MAYÚSCULAS o declarados con %token
                    // No-terminales en minúsculas
                    bool isT = std::all_of(token.begin(), token.end(),
                                           [](char c){ return isupper(c) || c=='_'; });
                    // También verificamos si fue declarado como terminal
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
    j << "  ],\n  \"start\": \"" << startSymbol_.name << "\"\n}";
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
