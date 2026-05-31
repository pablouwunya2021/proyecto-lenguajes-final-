#include "yapar/slr1_table.hpp"
#include <iostream>
#include <sstream>

namespace yapar {

std::string Action::toString() const {
    switch (type) {
        case ActionType::SHIFT:  return "s" + std::to_string(value);
        case ActionType::REDUCE: return "r" + std::to_string(value);
        case ActionType::ACCEPT: return "acc";
        case ActionType::ERROR:  return "err";
    }
    return "err";
}

void SLR1Table::setAction(int state, const std::string& sym, const Action& a) {
    auto it = action_[state].find(sym);
    if (it != action_[state].end() && !(it->second.type == a.type && it->second.value == a.value)) {
        // Conflicto detectado
        Conflict c;
        c.state       = state;
        c.symbol      = sym;
        c.existing    = it->second;
        c.incoming    = a;
        c.description = "Conflicto en estado " + std::to_string(state) +
                        " con '" + sym + "': " +
                        it->second.toString() + " vs " + a.toString();
        conflicts_.push_back(c);
        // Por defecto preferimos shift sobre reduce
        if (a.type == ActionType::SHIFT) action_[state][sym] = a;
    } else {
        action_[state][sym] = a;
    }
}

void SLR1Table::build(const LR0Automaton& automaton) {
    automaton_  = &automaton;
    action_.clear();
    goto_.clear();
    conflicts_.clear();

    const Grammar&                grammar = automaton.getGrammar();
    const std::vector<Production>& prods  = grammar.getProductions();
    const std::vector<LR0State>&   states = automaton.getStates();

    for (const auto& state : states) {
        int s = state.id;

        // ── Transiciones de GOTO (no-terminales) ─────────────
        for (const auto& [sym, dest] : state.transitions) {
            // ¿Es terminal o no-terminal?
            bool isT = false;
            for (const auto& t : grammar.getTerminals())
                if (t.name == sym) { isT = true; break; }

            if (!isT) {
                goto_[s][sym] = dest;
            }
        }

        // ── Ítems del estado ──────────────────────────────────
        for (const auto& item : state.items) {
            if (item.prodId >= (int)prods.size()) continue;
            const Production& prod = prods[item.prodId];

            if (item.dotPos < (int)prod.rhs.size()) {
                // Dot NO está al final: posible SHIFT
                const Symbol& next = prod.rhs[item.dotPos];
                if (next.isTerminal) {
                    // ACTION[s][next] = shift(dest)
                    auto it = state.transitions.find(next.name);
                    if (it != state.transitions.end())
                        setAction(s, next.name, Action::shift(it->second));
                }
            } else {
                // Dot al final: REDUCE o ACCEPT
                // ACCEPT: cualquier producción del símbolo inicial con dot al final
                if (prod.lhs.name == grammar.getStartSymbol().name) {
                    setAction(s, END_MARKER, Action::accept());
                } else {
                    // ACTION[s][a] = reduce(prod) para todo a ∈ FOLLOW(prod.lhs)
                    for (const auto& a : grammar.getFollow(prod.lhs.name)) {
                        setAction(s, a, Action::reduce(item.prodId));
                    }
                }
            }
        }
    }
}

Action SLR1Table::getAction(int state, const std::string& terminal) const {
    auto si = action_.find(state);
    if (si == action_.end()) return Action::error();
    auto ai = si->second.find(terminal);
    if (ai == si->second.end()) return Action::error();
    return ai->second;
}

int SLR1Table::getGoto(int state, const std::string& nonTerminal) const {
    auto si = goto_.find(state);
    if (si == goto_.end()) return -1;
    auto gi = si->second.find(nonTerminal);
    if (gi == si->second.end()) return -1;
    return gi->second;
}

std::string SLR1Table::toJSON() const {
    std::ostringstream j;
    j << "{\n  \"action\": [\n";
    bool first = true;
    for (const auto& [state, row] : action_) {
        for (const auto& [sym, act] : row) {
            if (!first) j << ",\n"; first = false;
            j << "    {\"state\":" << state << ",\"symbol\":\"" << sym
              << "\",\"action\":\"" << act.toString() << "\"}";
        }
    }
    j << "\n  ],\n  \"goto\": [\n";
    first = true;
    for (const auto& [state, row] : goto_) {
        for (const auto& [sym, dest] : row) {
            if (!first) j << ",\n"; first = false;
            j << "    {\"state\":" << state << ",\"symbol\":\"" << sym
              << "\",\"goto\":" << dest << "}";
        }
    }
    j << "\n  ],\n  \"conflicts\": " << (conflicts_.empty() ? "[]" : "[");
    for (size_t i = 0; i < conflicts_.size(); i++) {
        if (i) j << ",";
        j << "\n    {\"state\":" << conflicts_[i].state
          << ",\"symbol\":\"" << conflicts_[i].symbol
          << "\",\"description\":\"" << conflicts_[i].description << "\"}";
    }
    if (!conflicts_.empty()) j << "\n  ]";
    j << "\n}";
    return j.str();
}

void SLR1Table::print() const {
    std::cout << "=== Tabla SLR(1) ===\n";
    if (!conflicts_.empty()) {
        std::cout << "⚠️  " << conflicts_.size() << " conflicto(s):\n";
        for (const auto& c : conflicts_) std::cout << "  " << c.description << "\n";
    }
    std::cout << "\nACTION:\n";
    for (const auto& [state, row] : action_) {
        std::cout << "  Estado " << state << ": ";
        for (const auto& [sym, act] : row)
            std::cout << sym << "=" << act.toString() << " ";
        std::cout << "\n";
    }
    std::cout << "\nGOTO:\n";
    for (const auto& [state, row] : goto_) {
        std::cout << "  Estado " << state << ": ";
        for (const auto& [sym, dest] : row)
            std::cout << sym << "=" << dest << " ";
        std::cout << "\n";
    }
}

} // namespace yapar
