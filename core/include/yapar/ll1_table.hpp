// ============================================================
//  ll1_table.hpp — Tabla LL(1)
//
//  Un parser LL(1) es top-down: parte del símbolo inicial
//  y expande usando la tabla M[A][a] donde:
//    A = no-terminal en el tope de la pila
//    a = terminal actual del input
//
//  M[A][a] = A → α  si:
//    a ∈ FIRST(α), o
//    ε ∈ FIRST(α) y a ∈ FOLLOW(A)
//
//  Una gramática es LL(1) si la tabla no tiene conflictos
//  (ninguna celda tiene más de una producción).
// ============================================================
#pragma once
#include "yapar/grammar.hpp"
#include <map>
#include <string>
#include <vector>

namespace yapar {

struct LL1Conflict {
    std::string nonTerminal;
    std::string terminal;
    std::vector<int> productions;  // IDs de las producciones en conflicto
};

class LL1Table {
public:
    void build(const Grammar& grammar);

    // M[nonTerminal][terminal] → id de producción (-1 si no existe)
    int getEntry(const std::string& nonTerminal,
                 const std::string& terminal) const;

    bool hasConflicts() const { return !conflicts_.empty(); }
    const std::vector<LL1Conflict>& getConflicts() const { return conflicts_; }

    std::string toJSON() const;
    void        print()  const;

private:
    std::map<std::string, std::map<std::string, int>> table_;
    std::vector<LL1Conflict> conflicts_;
    const Grammar* grammar_ = nullptr;

    void setEntry(const std::string& nt, const std::string& t, int prodId);
};

} // namespace yapar
