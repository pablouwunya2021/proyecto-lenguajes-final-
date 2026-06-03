// ============================================================
//  lalr_table.hpp — Tabla LALR(1)
//
//  LALR = LR(0) + lookaheads calculados por propagación.
//
//  Diferencia con SLR(1):
//    SLR usa FOLLOW(A) completo para las reducciones.
//    LALR calcula lookaheads más precisos por estado,
//    por eso tiene menos conflictos que SLR.
//
//  Algoritmo:
//  1. Construir el autómata LR(0)
//  2. Para cada ítem núcleo, calcular lookaheads espontáneos
//     y relaciones de propagación
//  3. Propagar los lookaheads hasta punto fijo
//  4. Construir la tabla igual que SLR pero usando los
//     lookaheads por ítem en lugar de FOLLOW global
// ============================================================
#pragma once
#include "yapar/lr0_automaton.hpp"
#include "yapar/slr1_table.hpp"
#include <map>
#include <set>
#include <string>

namespace yapar {

// ── Ítem LR(1): ítem LR(0) + lookahead ───────────────────────
struct LR1Item {
    LR0Item     base;
    std::string lookahead;

    bool operator<(const LR1Item& o) const {
        if (!(base == o.base)) return base < o.base;
        return lookahead < o.lookahead;
    }
};

class LALRTable {
public:
    void build(const LR0Automaton& automaton);

    Action getAction(int state, const std::string& terminal) const;
    int    getGoto(int state, const std::string& nonTerminal) const;

    bool hasConflicts() const { return !conflicts_.empty(); }
    const std::vector<Conflict>& getConflicts()         const { return conflicts_; }
    const std::vector<Conflict>& getResolvedConflicts() const { return resolvedConflicts_; }

    std::string toJSON() const;
    void        print()  const;

private:
    std::map<int, std::map<std::string, Action>> action_;
    std::map<int, std::map<std::string, int>>    goto_;
    std::vector<Conflict>                        conflicts_;
    std::vector<Conflict>                        resolvedConflicts_;
    const LR0Automaton*                          automaton_ = nullptr;

    // lookaheads_[estado][item] = conjunto de lookaheads
    std::map<int, std::map<LR0Item, std::set<std::string>>> lookaheads_;

    // Calcula lookaheads espontáneos y propagaciones
    void computeLookaheads();
    void propagate();

    // closure LR(1)
    std::set<LR1Item> closureLR1(const std::set<LR1Item>& items) const;

    void setAction(int state, const std::string& sym, const Action& a);
};

} // namespace yapar
