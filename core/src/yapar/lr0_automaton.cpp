// ============================================================
//  lr0_automaton.cpp — Construcción del autómata LR(0)
// ============================================================
#include "yapar/lr0_automaton.hpp"
#include <iostream>
#include <sstream>
#include <queue>
#include <algorithm>

namespace yapar {

// ════════════════════════════════════════════════════════════
//  symbolAfterDot — símbolo que está justo después del punto
//  Devuelve "" si el dot está al final (ítem completo)
// ════════════════════════════════════════════════════════════
std::string LR0Automaton::symbolAfterDot(const LR0Item& item) const {
    const Production& prod = grammar_->getProductions()[item.prodId];
    if (item.dotPos >= (int)prod.rhs.size()) return "";
    return prod.rhs[item.dotPos].name;
}

// ════════════════════════════════════════════════════════════
//  closure(I) — cierra un conjunto de ítems
//
//  Regla: si A → α • B β está en I y B es no-terminal,
//         agregamos todos los ítems B → • γ
//
//  Ejemplo:
//    I = { E' → • E }
//    B = E
//    Producciones de E: E → E+T, E → T
//    Resultado: { E' → • E, E → • E+T, E → • T }
//    Luego por E → • T: T → • T*F, T → • F
//    ... hasta punto fijo
// ════════════════════════════════════════════════════════════
std::set<LR0Item> LR0Automaton::closure(const std::set<LR0Item>& items) const {
    std::set<LR0Item> result = items;
    std::queue<LR0Item> worklist;
    for (const auto& item : items) worklist.push(item);

    while (!worklist.empty()) {
        LR0Item current = worklist.front(); worklist.pop();

        // ¿Qué símbolo está después del punto?
        std::string B = symbolAfterDot(current);
        if (B.empty()) continue; // dot al final, nada que expandir

        // ¿Es un no-terminal? Solo expandimos no-terminales
        bool isNT = false;
        for (const auto& nt : grammar_->getNonTerminals())
            if (nt.name == B) { isNT = true; break; }
        if (!isNT) continue;

        // Agregamos todos los ítems B → • γ
        for (int prodId : grammar_->getProductionsFor(B)) {
            LR0Item newItem{prodId, 0}; // dot al inicio
            if (!result.count(newItem)) {
                result.insert(newItem);
                worklist.push(newItem);
            }
        }
    }
    return result;
}

// ════════════════════════════════════════════════════════════
//  gotoItems(I, X) — transición del estado I con símbolo X
//
//  Resultado = closure({ A → α X • β | A → α • X β ∈ I })
//  Es decir: tomamos todos los ítems donde el símbolo después
//  del punto es X, avanzamos el punto, y cerramos.
// ════════════════════════════════════════════════════════════
std::set<LR0Item> LR0Automaton::gotoItems(const std::set<LR0Item>& items,
                                            const std::string& symbol) const {
    std::set<LR0Item> kernel; // ítems donde avanzamos el punto

    for (const auto& item : items) {
        if (symbolAfterDot(item) == symbol) {
            // Avanzamos el punto: dotPos + 1
            kernel.insert({item.prodId, item.dotPos + 1});
        }
    }

    if (kernel.empty()) return {};
    return closure(kernel);
}

// ════════════════════════════════════════════════════════════
//  findOrCreateState — busca un estado existente con ese
//  conjunto de ítems, o crea uno nuevo si no existe
// ════════════════════════════════════════════════════════════
int LR0Automaton::findOrCreateState(const std::set<LR0Item>& items) {
    for (const auto& state : states_)
        if (state.items == items) return state.id;

    // No existe → crear nuevo estado
    LR0State newState;
    newState.id    = (int)states_.size();
    newState.items = items;
    states_.push_back(newState);
    return newState.id;
}

// ════════════════════════════════════════════════════════════
//  build() — construye el autómata LR(0) completo
//
//  Algoritmo:
//  1. Crear la gramática aumentada: S' → S
//  2. Estado inicial = closure({ S' → • S })
//  3. Para cada estado I y cada símbolo X:
//     J = goto(I, X)
//     Si J no está vacío y no existe, crear estado para J
//     Agregar transición I --X--> J
//  4. Repetir hasta que no haya estados nuevos
// ════════════════════════════════════════════════════════════
void LR0Automaton::build(const Grammar& grammar) {
    grammar_ = &grammar;
    states_.clear();

    const auto& prods = grammar.getProductions();
    if (prods.empty()) throw std::runtime_error("Gramática vacía");

    // ── Gramática aumentada ───────────────────────────────────
    // Agregamos la producción especial S' → S al inicio
    // Para esto, identificamos el símbolo inicial
    // La producción aumentada tiene id = -1 (la usamos internamente)
    augmented_.id  = -1;
    augmented_.lhs = Symbol::nonTerminal(grammar.getStartSymbol().name + "'");
    augmented_.rhs = {grammar.getStartSymbol()};

    // El ítem inicial es: S' → • S (prodId=-1, dotPos=0)
    // Como no podemos indexar con -1, usamos el índice 0 de productions_
    // junto con la convención de que prodId=0 es la producción aumentada
    // Workaround: creamos una copia local de las producciones incluyendo la aumentada
    // Para simplificar, insertamos la aumentada como la producción 0
    // y renumeramos las demás (pero eso rompe los ids existentes).
    // Mejor: agregamos la producción aumentada con id = prods.size()
    int augId = (int)prods.size();

    // Creamos el ítem inicial
    LR0Item startItem{augId, 0};

    // Para que closure() funcione con prodId=augId, necesitamos que
    // getProductions()[augId] exista. Usamos una referencia a la gramática
    // pero agregamos la producción aumentada temporalmente en un vector local.
    // Solución elegante: grammar_ apunta a una Grammar modificada.
    // En su lugar, lo manejamos directamente en closure y symbolAfterDot
    // con un caso especial para prodId == augId.

    // Reservamos espacio generoso para evitar realloc durante el BFS.
    states_.reserve(512);

    // Estado 0: closure del ítem inicial (primera producción, dot al inicio)
    LR0Item initialItem{0, 0};
    std::set<LR0Item> initialItems = closure({initialItem});
    findOrCreateState(initialItems); // siempre crea el estado 0

    // ── BFS sobre los estados ─────────────────────────────────
    // Usamos índices enteros, nunca referencias/punteros a states_,
    // porque push_back puede realloc el vector.
    std::queue<int> stateQueue;
    stateQueue.push(0);
    std::set<int> processed;

    while (!stateQueue.empty()) {
        int stateId = stateQueue.front(); stateQueue.pop();
        if (processed.count(stateId)) continue;
        processed.insert(stateId);

        // Copia defensiva: evita dangling reference si states_ realloca
        std::set<LR0Item> items = states_[stateId].items;

        // Recolectamos todos los símbolos después del punto
        std::set<std::string> symbols;
        for (const auto& item : items) {
            std::string sym = symbolAfterDot(item);
            if (!sym.empty()) symbols.insert(sym);
        }

        // Para cada símbolo calculamos GOTO
        for (const auto& sym : symbols) {
            std::set<LR0Item> nextItems = gotoItems(items, sym);
            if (nextItems.empty()) continue;

            int nextId = findOrCreateState(nextItems);
            // Acceso por índice — seguro aunque states_ haya realloc
            states_[stateId].transitions[sym] = nextId;

            // Si es un estado nuevo, lo ponemos en la cola
            if (!processed.count(nextId)) {
                stateQueue.push(nextId);
            }
        }
    }

    // Marcar estados de aceptación:
    // El estado de aceptación es donde tenemos S → ... • (dot al final
    // de la primera producción — el símbolo inicial)
    for (auto& state : states_) {
        for (const auto& item : state.items) {
            const Production& prod = grammar.getProductions()[item.prodId];
            if (prod.lhs.name == grammar.getStartSymbol().name &&
                item.dotPos == (int)prod.rhs.size()) {
                state.isAccepting = true;
            }
        }
    }
}

bool LR0State::hasCompleteItems() const {
    // Un ítem es completo si el dot está al final del RHS
    // No podemos verificar aquí sin la gramática, así que
    // se verifica externamente; este método se usa como indicador general.
    return isAccepting;
}

// ════════════════════════════════════════════════════════════
//  toJSON — para el frontend D3.js
// ════════════════════════════════════════════════════════════
std::string LR0Automaton::toJSON() const {
    std::ostringstream j;
    j << "{\n  \"states\": [\n";

    for (size_t i = 0; i < states_.size(); i++) {
        const auto& st = states_[i];
        j << "    {\"id\":" << st.id << ",\"accepting\":"
          << (st.isAccepting?"true":"false") << ",\"items\":[\n";

        bool firstItem = true;
        for (const auto& item : st.items) {
            if (!firstItem) j << ",\n"; firstItem = false;
            const auto& prod = grammar_->getProductions()[item.prodId];
            j << "      \"" << prod.lhs.name << " → ";
            for (int k = 0; k <= (int)prod.rhs.size(); k++) {
                if (k == item.dotPos) j << "• ";
                if (k < (int)prod.rhs.size()) j << prod.rhs[k].name << " ";
            }
            j << "\"";
        }
        j << "\n    ]}";
        if (i+1 < states_.size()) j << ",";
        j << "\n";
    }

    j << "  ],\n  \"transitions\": [\n";
    bool firstTrans = true;
    for (const auto& st : states_) {
        for (const auto& [sym, dest] : st.transitions) {
            if (!firstTrans) j << ",\n"; firstTrans = false;
            j << "    {\"from\":" << st.id << ",\"to\":" << dest
              << ",\"label\":\"" << sym << "\"}";
        }
    }
    j << "\n  ]\n}";
    return j.str();
}

void LR0Automaton::print() const {
    std::cout << "=== Autómata LR(0) (" << states_.size() << " estados) ===\n\n";
    for (const auto& st : states_) {
        std::cout << "Estado " << st.id;
        if (st.isAccepting) std::cout << " [ACEPTA]";
        std::cout << ":\n";
        for (const auto& item : st.items) {
            const auto& prod = grammar_->getProductions()[item.prodId];
            std::cout << "  " << prod.lhs.name << " → ";
            for (int k = 0; k <= (int)prod.rhs.size(); k++) {
                if (k == item.dotPos) std::cout << "• ";
                if (k < (int)prod.rhs.size()) std::cout << prod.rhs[k].name << " ";
            }
            std::cout << "\n";
        }
        for (const auto& [sym, dest] : st.transitions)
            std::cout << "  --" << sym << "--> " << dest << "\n";
        std::cout << "\n";
    }
}

} // namespace yapar
