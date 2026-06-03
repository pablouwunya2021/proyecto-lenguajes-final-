// ============================================================
//  lr0_automaton.hpp — Autómata LR(0)
//
//  Un ítem LR(0) es una producción con un punto •:
//    A → α • β
//  El punto indica cuánto hemos visto (α) y qué esperamos (β).
//
//  Un estado LR(0) es un conjunto de ítems cerrado bajo closure:
//    Si tenemos A → α • B β, agregamos todos B → • γ
//
//  Las transiciones entre estados se calculan con GOTO(I, X):
//    Para cada ítem A → α • X β en I,
//    el nuevo estado contiene closure({ A → α X • β })
// ============================================================
#pragma once
#include "yapar/grammar.hpp"
#include <vector>
#include <set>
#include <map>
#include <string>

namespace yapar {

// ── Ítem LR(0) ───────────────────────────────────────────────
struct LR0Item {
    int prodId;   // índice de la producción en la gramática
    int dotPos;   // posición del punto (0 = inicio, rhs.size() = final)

    bool operator==(const LR0Item& o) const {
        return prodId == o.prodId && dotPos == o.dotPos;
    }
    bool operator<(const LR0Item& o) const {
        if (prodId != o.prodId) return prodId < o.prodId;
        return dotPos < o.dotPos;
    }
};

// ── Estado del autómata LR(0) ────────────────────────────────
struct LR0State {
    int              id;
    std::set<LR0Item> items;   // conjunto de ítems (cerrado)

    // Transiciones: símbolo → estado destino
    std::map<std::string, int> transitions;

    // ¿Tiene ítems completos (dot al final)?
    bool hasCompleteItems() const;

    // Tipo del estado para el reporte
    bool isAccepting = false;
};

// ── Autómata LR(0) ───────────────────────────────────────────
class LR0Automaton {
public:
    // Construye el autómata desde una gramática aumentada
    // (agrega S' → • S automáticamente)
    void build(const Grammar& grammar);

    const std::vector<LR0State>& getStates()      const { return states_; }
    const Grammar&               getGrammar()      const { return *grammar_; }
    int                          getStartState()   const { return 0; }

    // Serialización para D3.js en el frontend
    std::string toJSON() const;
    std::string toDOT(const std::string& title = "LR(0)") const;
    void        print()  const;

private:
    std::vector<LR0State> states_;
    const Grammar*        grammar_ = nullptr;
    Production            augmented_;   // S' → S

    // ── Algoritmos centrales ──────────────────────────────

    // closure(I): cierra un conjunto de ítems bajo la regla:
    //   si A → α • B β está en I, agrega todos B → • γ
    std::set<LR0Item> closure(const std::set<LR0Item>& items) const;

    // goto(I, X): ítems que resultan de "avanzar" sobre el símbolo X
    std::set<LR0Item> gotoItems(const std::set<LR0Item>& items,
                                 const std::string& symbol) const;

    // Busca o crea el estado para un conjunto de ítems
    int findOrCreateState(const std::set<LR0Item>& items);

    // Símbolo después del punto (el que "esperamos")
    // Devuelve "" si el dot está al final
    std::string symbolAfterDot(const LR0Item& item) const;
};

} // namespace yapar
