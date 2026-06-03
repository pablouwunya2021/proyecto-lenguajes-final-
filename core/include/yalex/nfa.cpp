// ============================================================
//  nfa.cpp
//  Implementación de Thompson's Construction
//
//  El algoritmo funciona con una PILA de fragmentos NFA.
//  Recorremos la expresión postfix de izquierda a derecha:
//    - Si vemos un LITERAL → construimos fragmento, lo apilamos
//    - Si vemos un operador unario (*, +, ?) → sacamos 1 fragmento,
//      aplicamos el operador, apilamos el resultado
//    - Si vemos un operador binario (|, ·) → sacamos 2 fragmentos,
//      los combinamos, apilamos el resultado
//  Al final la pila tiene exactamente 1 fragmento: el NFA completo.
// ============================================================

#include "yalex/nfa.hpp"
#include <stack>
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace yalex {

// ════════════════════════════════════════════════════════════
//  Helpers básicos
// ════════════════════════════════════════════════════════════

// Crea un nuevo estado vacío y lo agrega al pool
int NFA::newState() {
    states_.emplace_back(nextStateId_);
    return nextStateId_++;
}

// Agrega una arista dirigida from→to con etiqueta c
void NFA::addTransition(int from, int to, char c) {
    // Nos aseguramos de que el vector tenga suficiente espacio
    while ((int)states_.size() <= std::max(from, to)) {
        states_.emplace_back(nextStateId_++);
    }
    states_[from].transitions[c].insert(to);
}


// ════════════════════════════════════════════════════════════
//  Fragmentos de Thompson's Construction
// ════════════════════════════════════════════════════════════

// ── buildLiteral: fragmento para un caracter exacto ──────────
//
//   q0 ──'c'──▶ q1
//
NFAFragment NFA::buildLiteral(char c) {
    int q0 = newState();
    int q1 = newState();
    addTransition(q0, q1, c);
    return NFAFragment(q0, {q1});
}

// ── buildEpsilon: fragmento para ε ───────────────────────────
//
//   q0 ──ε──▶ q1
//
NFAFragment NFA::buildEpsilon() {
    int q0 = newState();
    int q1 = newState();
    addTransition(q0, q1, EPSILON_CHAR);
    return NFAFragment(q0, {q1});
}

// ── buildAny: fragmento para el punto '.' ────────────────────
//  Reconoce cualquier caracter imprimible (ASCII 32-126)
NFAFragment NFA::buildAny() {
    int q0 = newState();
    int q1 = newState();
    for (char c = 32; c < 127; c++) {
        addTransition(q0, q1, c);
    }
    return NFAFragment(q0, {q1});
}

// ── buildUnion: fragmento para A|B ───────────────────────────
//
//            ε ──▶ [NFA_A] ──ε──▶
//   q_new ──                      q_final
//            ε ──▶ [NFA_B] ──ε──▶
//
NFAFragment NFA::buildUnion(NFAFragment a, NFAFragment b) {
    int q_new   = newState();
    int q_final = newState();

    addTransition(q_new, a.start, EPSILON_CHAR);
    addTransition(q_new, b.start, EPSILON_CHAR);

    for (int s : a.accepting)
        addTransition(s, q_final, EPSILON_CHAR);

    for (int s : b.accepting)
        addTransition(s, q_final, EPSILON_CHAR);

    return NFAFragment(q_new, {q_final});
}

// ── buildConcat: fragmento para A·B ──────────────────────────
//
//   [NFA_A] ──ε──▶ [NFA_B]
//
NFAFragment NFA::buildConcat(NFAFragment a, NFAFragment b) {
    for (int s : a.accepting)
        addTransition(s, b.start, EPSILON_CHAR);

    return NFAFragment(a.start, b.accepting);
}

// ── buildKleeneStar: fragmento para A* ───────────────────────
//
//            ε ──────────────────────▶
//   q_new ──ε──▶ [NFA_A] ──ε──▶ q_final
//                    ↑──────ε────┘
//
NFAFragment NFA::buildKleeneStar(NFAFragment a) {
    int q_new   = newState();
    int q_final = newState();

    addTransition(q_new, a.start,  EPSILON_CHAR);
    addTransition(q_new, q_final,  EPSILON_CHAR);

    for (int s : a.accepting) {
        addTransition(s, a.start,  EPSILON_CHAR);
        addTransition(s, q_final,  EPSILON_CHAR);
    }

    return NFAFragment(q_new, {q_final});
}

// ── buildPlus: fragmento para A+ ─────────────────────────────
//
//   [NFA_A] con bucle de regreso, sin ε inicial (exige al menos 1)
//
NFAFragment NFA::buildPlus(NFAFragment a) {
    int q_final = newState();

    for (int s : a.accepting) {
        addTransition(s, a.start, EPSILON_CHAR);
        addTransition(s, q_final, EPSILON_CHAR);
    }

    return NFAFragment(a.start, {q_final});
}

// ── buildOptional: fragmento para A? ─────────────────────────
//
//   q_new ──ε──▶ [NFA_A] ──ε──▶ q_final
//         └────────────────ε────▶
//
NFAFragment NFA::buildOptional(NFAFragment a) {
    int q_new   = newState();
    int q_final = newState();

    addTransition(q_new, a.start,  EPSILON_CHAR);
    addTransition(q_new, q_final,  EPSILON_CHAR);

    for (int s : a.accepting)
        addTransition(s, q_final, EPSILON_CHAR);

    return NFAFragment(q_new, {q_final});
}


// ════════════════════════════════════════════════════════════
//  build() — orquesta Thompson's Construction sobre postfix
// ════════════════════════════════════════════════════════════
void NFA::build(const std::vector<RegexToken>& postfix,
                const std::string& tokenName) {
    std::stack<NFAFragment> stack;

    for (const auto& token : postfix) {
        switch (token.type) {

            case RegexTokenType::LITERAL:
                stack.push(buildLiteral(token.value));
                break;

            case RegexTokenType::ANY:
                stack.push(buildAny());
                break;

            case RegexTokenType::EPSILON:
                stack.push(buildEpsilon());
                break;

            case RegexTokenType::KLEENE_STAR: {
                if (stack.empty())
                    throw std::invalid_argument("* sin operando");
                auto a = stack.top(); stack.pop();
                stack.push(buildKleeneStar(a));
                break;
            }

            case RegexTokenType::PLUS: {
                if (stack.empty())
                    throw std::invalid_argument("+ sin operando");
                auto a = stack.top(); stack.pop();
                stack.push(buildPlus(a));
                break;
            }

            case RegexTokenType::OPTIONAL: {
                if (stack.empty())
                    throw std::invalid_argument("? sin operando");
                auto a = stack.top(); stack.pop();
                stack.push(buildOptional(a));
                break;
            }

            case RegexTokenType::CONCAT: {
                if (stack.size() < 2)
                    throw std::invalid_argument("· sin suficientes operandos");
                auto b = stack.top(); stack.pop(); // derecho
                auto a = stack.top(); stack.pop(); // izquierdo
                stack.push(buildConcat(a, b));
                break;
            }

            case RegexTokenType::UNION: {
                if (stack.size() < 2)
                    throw std::invalid_argument("| sin suficientes operandos");
                auto b = stack.top(); stack.pop();
                auto a = stack.top(); stack.pop();
                stack.push(buildUnion(a, b));
                break;
            }

            default:
                break;
        }
    }

    if (stack.size() != 1)
        throw std::invalid_argument(
            "Expresión regular mal formada (postfix inválido)"
        );

    NFAFragment result = stack.top();
    startState_ = result.start;

    // Marcamos el único estado de aceptación
    if (result.accepting.size() != 1)
        throw std::logic_error(
            "El NFA debe tener exactamente 1 estado de aceptación"
        );

    acceptingState_ = *result.accepting.begin();
    states_[acceptingState_].isAccepting = true;
    states_[acceptingState_].tokenName   = tokenName;
}


// ════════════════════════════════════════════════════════════
//  epsilonClosure() — todos los estados alcanzables por ε
// ════════════════════════════════════════════════════════════
std::set<int> NFA::epsilonClosure(const std::set<int>& states) const {
    std::set<int>   closure = states;
    std::stack<int> worklist;

    for (int s : states)
        worklist.push(s);

    while (!worklist.empty()) {
        int current = worklist.top();
        worklist.pop();

        if (current < 0 || current >= (int)states_.size()) continue;

        auto it = states_[current].transitions.find(EPSILON_CHAR);
        if (it != states_[current].transitions.end()) {
            for (int next : it->second) {
                if (closure.find(next) == closure.end()) {
                    closure.insert(next);
                    worklist.push(next);
                }
            }
        }
    }

    return closure;
}


// ════════════════════════════════════════════════════════════
//  move() — transición sobre un caracter c
// ════════════════════════════════════════════════════════════
std::set<int> NFA::move(const std::set<int>& states, char c) const {
    std::set<int> result;

    for (int s : states) {
        if (s < 0 || s >= (int)states_.size()) continue;
        auto it = states_[s].transitions.find(c);
        if (it != states_[s].transitions.end()) {
            result.insert(it->second.begin(), it->second.end());
        }
    }

    return result;
}


// ════════════════════════════════════════════════════════════
//  print() — debug visual en consola
// ════════════════════════════════════════════════════════════
void NFA::print() const {
    std::cout << "=== NFA (" << states_.size() << " estados) ===\n";
    std::cout << "Inicio: q" << startState_ << "\n";
    if (acceptingState_ >= 0)
        std::cout << "Aceptacion: q" << acceptingState_
                  << " [" << states_[acceptingState_].tokenName << "]\n\n";

    for (const auto& state : states_) {
        std::cout << "q" << state.id;
        if (state.isAccepting)
            std::cout << " [ACEPTA: " << state.tokenName << "]";
        std::cout << ":\n";

        for (const auto& [c, targets] : state.transitions) {
            std::cout << "  --";
            if (c == EPSILON_CHAR) std::cout << "ε";
            else std::cout << "'" << c << "'";
            std::cout << "--> { ";
            for (int t : targets) std::cout << "q" << t << " ";
            std::cout << "}\n";
        }
    }
    std::cout << "\n";
}


// ════════════════════════════════════════════════════════════
//  CombinedNFA — une múltiples NFAs con un super-estado inicial
//
//  Resultado:
//              ε ──▶ [NFA_ID]
//  [superQ0] ──ε ──▶ [NFA_NUMBER]
//              ε ──▶ [NFA_PLUS]
//
//  Este super-NFA es el que luego convierte el DFA.
// ════════════════════════════════════════════════════════════
void CombinedNFA::addRule(NFA nfa, const std::string& tokenName) {
    rules_.emplace_back(std::move(nfa), tokenName);
}

NFA CombinedNFA::combine() {
    if (rules_.empty())
        throw std::runtime_error("CombinedNFA: no hay reglas que combinar");

    NFA combined;

    // Estado 0 = super-estado inicial
    int superStart = combined.newState();

    // offset: como cada NFA individual tiene IDs empezando en 0,
    // necesitamos desplazarlos para que no colisionen entre sí.
    // El estado 0 ya está tomado por superStart, así que offset = 1.
    int offset = 1;

    for (auto& [nfa, tokenName] : rules_) {
        const auto& nfaStates = nfa.getStates();
        int         nfaStart  = nfa.getStartState();

        // Expandimos el pool de estados del combined hasta el tamaño necesario
        int maxNewId = offset + (int)nfaStates.size() - 1;
        while ((int)combined.states_.size() <= maxNewId) {
            combined.states_.emplace_back(combined.nextStateId_++);
        }

        // Copiamos cada estado del NFA individual al combined
        for (const auto& state : nfaStates) {
            int newId = state.id + offset;

            // Copiamos propiedades del estado
            combined.states_[newId].isAccepting = state.isAccepting;
            combined.states_[newId].tokenName   = state.tokenName;

            // Copiamos transiciones remapeando los IDs destino
            for (const auto& [c, targets] : state.transitions) {
                for (int target : targets) {
                    combined.states_[newId].transitions[c].insert(target + offset);
                }
            }
        }

        // ε-transición del super-estado al inicio de este NFA
        combined.states_[superStart].transitions[EPSILON_CHAR]
            .insert(nfaStart + offset);

        // Avanzamos el offset para el siguiente NFA
        offset += static_cast<int>(nfaStates.size());
    }

    combined.startState_     = superStart;
    combined.acceptingState_ = -1; // cada sub-NFA tiene el suyo
    return combined;
}

} // namespace yalex