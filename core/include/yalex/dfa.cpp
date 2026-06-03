// ============================================================
//  dfa.hpp
//  Módulo: YALex — Autómata Finito Determinista (DFA)
//
//  ¿Qué es un DFA?
//  Un DFA es como el NFA pero con una regla clave:
//  desde cada estado, con cada caracter, hay EXACTAMENTE
//  una transición (o ninguna). Sin ambigüedad, sin ε.
//
//  ¿Cómo lo obtenemos?
//  Usamos "Subset Construction": cada estado del DFA
//  representa un CONJUNTO de estados del NFA.
//
//  Ejemplo con el NFA de a|b:
//
//    NFA:                    DFA:
//    q4 --ε--> q0,q2         D0={q4,q0,q2,q5}*  (ε-closure de q4)
//    q0 --a--> q1            D0 --'a'--> D1={q1,q5}*
//    q2 --b--> q3            D0 --'b'--> D2={q3,q5}*
//
//  Los estados DFA marcados con * aceptan (contienen estados
//  de aceptación del NFA).
//
//  Después de construir el DFA, lo MINIMIZAMOS con el
//  algoritmo de Hopcroft para reducir el número de estados.
// ============================================================

#pragma once

#include "yalex/nfa.hpp"
#include <vector>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <optional>

namespace yalex {

// ── Estado del DFA ────────────────────────────────────────────
// A diferencia del NFA, las transiciones del DFA son
// deterministas: char → UN SOLO estado destino (no un set).
struct DFAState {
    int  id;
    bool isAccepting;
    std::string tokenName;  // Qué token acepta (si isAccepting)
    int  priority;          // Para resolver conflictos: regla más
                            // temprana en el .yalex tiene prioridad

    // Transiciones: char → ID del estado destino (solo uno)
    std::map<char, int> transitions;

    // El conjunto de estados NFA que representa este estado DFA
    // (lo guardamos para debugging y minimización)
    std::set<int> nfaStates;

    explicit DFAState(int id)
        : id(id), isAccepting(false), priority(0) {}
};

// ── Resultado de ejecutar el DFA sobre un input ───────────────
struct DFAMatch {
    bool        matched;
    std::string tokenName;
    std::string lexeme;    // El texto exacto que hizo match
    int         line;      // Línea donde empieza (1-indexed)
    int         column;    // Columna donde empieza (1-indexed)
};

// ── Clase principal: DFA ──────────────────────────────────────
class DFA {
public:
    DFA() : startState_(0), deadState_(-1) {}

    // ── Construcción ──────────────────────────────────────────

    // buildFromNFA() — Subset Construction: NFA → DFA
    void buildFromNFA(const NFA& nfa);

    // minimize() — Algoritmo de Hopcroft
    void minimize();

    // ── Simulación ────────────────────────────────────────────

    // nextMatch() — busca el PRIMER token en 'input' a partir
    // de la posición 'pos'. Implementa maximal munch.
    std::optional<DFAMatch> nextMatch(
        const std::string& input,
        size_t& pos,
        int& line,
        int& col
    ) const;

    // Getters
    int getStartState() const { return startState_; }
    const std::vector<DFAState>& getStates() const { return states_; }

    // Serialización a JSON (para enviar al frontend)
    std::string toJSON() const;

    // Debug
    void print() const;

private:
    std::vector<DFAState> states_;
    int startState_;
    int deadState_;

    // ── Helpers de Subset Construction ───────────────────────
    std::string    setToKey(const std::set<int>& s) const;
    std::set<char> getAlphabet(const NFA& nfa) const;

    // ── Helpers de Minimización (Hopcroft) ───────────────────
    std::vector<std::set<int>> refine(
        const std::vector<std::set<int>>& partition,
        char c
    ) const;
};

} // namespace yalex