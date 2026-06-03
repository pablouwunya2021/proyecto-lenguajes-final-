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

// ── Precedencia / asociatividad ────────────────────────────
// Se declara con %left, %right, %nonassoc en el .yapar.
// El nivel sube con cada directiva (mayor nivel = mayor precedencia).
enum class Associativity { LEFT, RIGHT, NONASSOC, NONE };

struct PrecInfo {
    int           level;  // mayor = mayor precedencia
    Associativity assoc;
};

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
    int                 id;         // índice único en la gramática
    Symbol              lhs;        // lado izquierdo (no-terminal)
    std::vector<Symbol> rhs;        // lado derecho (lista de símbolos)
    int                 sourceLine = 0;  // línea en el .yapar donde se definió

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

    // ── Precedencia ────────────────────────────────────────
    // Retorna info de precedencia del token, o nullptr si no tiene.
    const PrecInfo* getPrecedence(const std::string& token) const;

    // Retorna info de precedencia de la producción.
    // Usa %prec override si existe; si no, el terminal más a la derecha del RHS.
    const PrecInfo* getProductionPrec(int prodId) const;

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

    // token → {nivel, asociatividad}
    std::map<std::string, PrecInfo> precMap_;
    // prodId → token cuya precedencia usar (declarado con %prec TOKEN)
    std::map<int, std::string>      prodPrecToken_;

    // true cuando se encontró el separador '%%' al leer el .yapar
    bool sawSeparator_ = false;

    // ── Parser del .yapar ──────────────────────────────────
    void parse(const std::string& content);
    void addSymbol(const Symbol& s);

    // Verifica coherencia de la gramática leída (lanza si hay errores)
    void validate() const;

    // ── Algoritmos ────────────────────────────────────────
    void computeFirst();
    void computeFollow();
};

} // namespace yapar
