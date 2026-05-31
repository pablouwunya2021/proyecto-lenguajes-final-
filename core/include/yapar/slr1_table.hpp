// ============================================================
//  slr1_table.hpp — Tabla SLR(1)
//
//  La tabla tiene dos partes:
//  ACTION[estado][terminal]:
//    - shift(s)   → avanza el input y empuja estado s
//    - reduce(p)  → aplica la producción p
//    - accept     → análisis exitoso
//    - error      → error sintáctico
//
//  GOTO[estado][no-terminal]:
//    - número de estado → después de reducir, ir a este estado
//
//  Reglas de construcción SLR(1):
//    - Si A → α • a β en estado i y GOTO(i,a)=j → ACTION[i][a]=shift(j)
//    - Si A → α • en estado i y a ∈ FOLLOW(A) → ACTION[i][a]=reduce(A→α)
//    - Si S' → S • en estado i → ACTION[i][$]=accept
// ============================================================
#pragma once
#include "yapar/lr0_automaton.hpp"
#include <map>
#include <string>
#include <variant>
#include <vector>

namespace yapar {

// ── Acción de la tabla ACTION ────────────────────────────────
enum class ActionType { SHIFT, REDUCE, ACCEPT, ERROR };

struct Action {
    ActionType type;
    int        value;  // para SHIFT: estado destino; para REDUCE: id producción

    static Action shift(int s)  { return {ActionType::SHIFT,  s}; }
    static Action reduce(int p) { return {ActionType::REDUCE, p}; }
    static Action accept()      { return {ActionType::ACCEPT, 0}; }
    static Action error()       { return {ActionType::ERROR,  0}; }

    std::string toString() const;
};

// ── Conflicto shift/reduce ────────────────────────────────────
struct Conflict {
    int         state;
    std::string symbol;
    Action      existing;
    Action      incoming;
    std::string description;
};

// ── Tabla SLR(1) ─────────────────────────────────────────────
class SLR1Table {
public:
    // Construye la tabla desde el autómata LR(0)
    void build(const LR0Automaton& automaton);

    // Consultas
    Action getAction(int state, const std::string& terminal) const;
    int    getGoto(int state, const std::string& nonTerminal) const;

    const std::vector<Conflict>& getConflicts() const { return conflicts_; }
    bool hasConflicts() const { return !conflicts_.empty(); }

    // Serialización
    std::string toJSON() const;
    void        print()  const;

private:
    // ACTION[estado][terminal] → acción
    std::map<int, std::map<std::string, Action>> action_;

    // GOTO[estado][no-terminal] → estado
    std::map<int, std::map<std::string, int>> goto_;

    std::vector<Conflict> conflicts_;
    const LR0Automaton*   automaton_ = nullptr;

    void setAction(int state, const std::string& sym, const Action& a);
};

} // namespace yapar
