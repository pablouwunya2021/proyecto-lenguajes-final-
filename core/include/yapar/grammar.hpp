// ============================================================
//  grammar.hpp — Gramática formal + FIRST / FOLLOW
// ============================================================
#pragma once
#include <string>
#include <vector>
#include <set>
#include <map>
#include <unordered_map>

namespace yapar {

// ── Símbolo ────────────────────────────────────────────────
// Un símbolo es terminal (token) o no-terminal (variable).
// Usamos una constante especial para epsilon y para $.
static const std::string EPSILON = "ε";
static const std::string END_MARKER = "$";   // fin de input

struct Symbol {
    std::string name;
    bool        isTerminal;

    bool operator==(const Symbol& o) const { return name == o.name; }
    bool operator<(const Symbol& o)  const { return name <  o.name; }

    static Symbol terminal(const std::string& n)    { return {n, true};  }
    static Symbol nonTerminal(const std::string& n) { return {n, false}; }
};

// ── Producción ──────────────────────────────────────────────
// Una regla de la forma: A → α  donde α es una lista de símbolos.
// Si α está vacío equivale a A → ε (producción epsilon).
struct Production {
    int                 id;     // índice único en la gramática
    Symbol              lhs;   // lado izquierdo (no-terminal)
    std::vector<Symbol> rhs;   // lado derecho (lista de símbolos)

    bool isEpsilon() const { return rhs.empty(); }
};

// ── Gramática ────────────────────────────────────────────────
class Grammar {
public:
    // ── Carga ──────────────────────────────────────────────
    void loadFromFile(const std::string& filepath);
    void loadFromString(const std::string& content);

    // ── Consultas ──────────────────────────────────────────
    const std::vector<Production>&           getProductions()  const { return productions_; }
    const Symbol&                            getStartSymbol()  const { return startSymbol_; }
    const std::vector<Symbol>&               getTerminals()    const { return terminals_; }
    const std::vector<Symbol>&               getNonTerminals() const { return nonTerminals_; }

    // Todas las producciones donde lhs.name == name
    std::vector<int> getProductionsFor(const std::string& name) const;

    // ── FIRST y FOLLOW ─────────────────────────────────────
    // FIRST(α): conjunto de terminales que pueden aparecer
    //           al inicio de cualquier cadena derivada de α
    // FOLLOW(A): conjunto de terminales que pueden aparecer
    //            DESPUÉS de A en alguna forma sentencial
    void computeFirstAndFollow();

    const std::set<std::string>& getFirst(const std::string& symbol)  const;
    const std::set<std::string>& getFollow(const std::string& symbol) const;

    // FIRST de una secuencia de símbolos α
    std::set<std::string> firstOfSequence(const std::vector<Symbol>& seq) const;

    // Serialización para el API
    std::string toJSON() const;
    void        print()  const;

private:
    std::vector<Production> productions_;
    Symbol                  startSymbol_{"", false};
    std::vector<Symbol>     terminals_;
    std::vector<Symbol>     nonTerminals_;

    // FIRST[X] y FOLLOW[X] indexados por nombre del símbolo
    std::map<std::string, std::set<std::string>> first_;
    std::map<std::string, std::set<std::string>> follow_;

    // ── Parser del .yapar ──────────────────────────────────
    void parse(const std::string& content);
    void addSymbol(const Symbol& s);

    // ── Algoritmos ────────────────────────────────────────
    void computeFirst();
    void computeFollow();
};

} // namespace yapar
