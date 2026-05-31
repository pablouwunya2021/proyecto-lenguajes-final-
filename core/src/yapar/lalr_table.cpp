#include "yapar/lalr_table.hpp"
#include <iostream>
#include <sstream>
#include <queue>

namespace yapar {

void LALRTable::setAction(int state, const std::string& sym, const Action& a) {
    auto it = action_[state].find(sym);
    if (it != action_[state].end() && !(it->second.type==a.type && it->second.value==a.value)) {
        Conflict c;
        c.state=state; c.symbol=sym; c.existing=it->second; c.incoming=a;
        c.description="LALR conflicto en estado "+std::to_string(state)+" '"+sym+"': "+
                       it->second.toString()+" vs "+a.toString();
        conflicts_.push_back(c);
        if (a.type == ActionType::SHIFT) action_[state][sym] = a;
    } else {
        action_[state][sym] = a;
    }
}

// closure LR(1): cierra ítems LR(1) = LR(0) item + lookahead
std::set<LR1Item> LALRTable::closureLR1(const std::set<LR1Item>& items) const {
    std::set<LR1Item> result = items;
    std::queue<LR1Item> wl;
    for (const auto& i : items) wl.push(i);

    const Grammar& g = automaton_->getGrammar();

    while (!wl.empty()) {
        LR1Item cur = wl.front(); wl.pop();
        const Production& prod = g.getProductions()[cur.base.prodId];
        if (cur.base.dotPos >= (int)prod.rhs.size()) continue;

        const Symbol& B = prod.rhs[cur.base.dotPos];
        if (B.isTerminal) continue;

        // β = lo que sigue a B + lookahead
        std::vector<Symbol> beta(prod.rhs.begin() + cur.base.dotPos + 1, prod.rhs.end());
        beta.push_back(Symbol::terminal(cur.lookahead));
        auto firstBeta = g.firstOfSequence(beta);

        for (int pid : g.getProductionsFor(B.name)) {
            for (const auto& la : firstBeta) {
                if (la == EPSILON) continue;
                LR1Item newItem{{pid, 0}, la};
                if (!result.count(newItem)) { result.insert(newItem); wl.push(newItem); }
            }
        }
    }
    return result;
}

void LALRTable::computeLookaheads() {
    const Grammar& g = automaton_->getGrammar();
    const auto& states = automaton_->getStates();

    // Inicializar lookahead del ítem inicial con $
    lookaheads_[0][{0, 0}].insert(END_MARKER);

    // Propagar lookaheads usando relaciones de propagación
    bool changed = true;
    while (changed) {
        changed = false;
        for (const auto& state : states) {
            int s = state.id;
            for (const auto& item : state.items) {
                auto& las = lookaheads_[s][item];
                if (las.empty()) continue;

                const Production& prod = g.getProductions()[item.prodId];
                if (item.dotPos >= (int)prod.rhs.size()) continue;

                const std::string& sym = prod.rhs[item.dotPos].name;
                auto destIt = state.transitions.find(sym);
                if (destIt == state.transitions.end()) continue;
                int dest = destIt->second;

                LR0Item destItem{item.prodId, item.dotPos + 1};

                // Los lookaheads se propagan al ítem destino
                for (const auto& la : las) {
                    changed |= lookaheads_[dest][destItem].insert(la).second;
                }

                // También propagar a los ítems del closure en el destino
                const auto& destState = states[dest];
                for (const auto& di : destState.items) {
                    const Production& dp = g.getProductions()[di.prodId];
                    if (di.dotPos == 0) {
                        // Ítem de closure: propagar lookaheads apropiados
                        for (const auto& la : las) {
                            changed |= lookaheads_[dest][di].insert(la).second;
                        }
                    }
                }
            }
        }
    }
}

void LALRTable::build(const LR0Automaton& automaton) {
    automaton_  = &automaton;
    action_.clear();
    goto_.clear();
    conflicts_.clear();
    lookaheads_.clear();

    const Grammar& grammar = automaton.getGrammar();
    const auto& prods  = grammar.getProductions();
    const auto& states = automaton.getStates();

    // Calcular lookaheads
    computeLookaheads();

    for (const auto& state : states) {
        int s = state.id;

        // GOTO (igual que SLR)
        for (const auto& [sym, dest] : state.transitions) {
            bool isT = false;
            for (const auto& t : grammar.getTerminals()) if (t.name==sym){isT=true;break;}
            if (!isT) goto_[s][sym] = dest;
        }

        for (const auto& item : state.items) {
            if (item.prodId >= (int)prods.size()) continue;
            const Production& prod = prods[item.prodId];

            if (item.dotPos < (int)prod.rhs.size()) {
                // SHIFT
                const Symbol& next = prod.rhs[item.dotPos];
                if (next.isTerminal) {
                    auto it = state.transitions.find(next.name);
                    if (it != state.transitions.end())
                        setAction(s, next.name, Action::shift(it->second));
                }
            } else {
                // REDUCE — usamos los lookaheads calculados
                if (prod.lhs.name == grammar.getStartSymbol().name && item.prodId == 0) {
                    setAction(s, END_MARKER, Action::accept());
                } else {
                    // Lookaheads específicos para este ítem en este estado
                    auto stateIt = lookaheads_.find(s);
                    if (stateIt != lookaheads_.end()) {
                        auto itemIt = stateIt->second.find(item);
                        if (itemIt != stateIt->second.end()) {
                            for (const auto& la : itemIt->second)
                                setAction(s, la, Action::reduce(item.prodId));
                        }
                    }
                    // Fallback: usar FOLLOW como SLR si no hay lookaheads
                    for (const auto& a : grammar.getFollow(prod.lhs.name))
                        setAction(s, a, Action::reduce(item.prodId));
                }
            }
        }
    }
}

Action LALRTable::getAction(int state, const std::string& terminal) const {
    auto si = action_.find(state);
    if (si==action_.end()) return Action::error();
    auto ai = si->second.find(terminal);
    if (ai==si->second.end()) return Action::error();
    return ai->second;
}

int LALRTable::getGoto(int state, const std::string& nonTerminal) const {
    auto si = goto_.find(state);
    if (si==goto_.end()) return -1;
    auto gi = si->second.find(nonTerminal);
    if (gi==si->second.end()) return -1;
    return gi->second;
}

std::string LALRTable::toJSON() const {
    std::ostringstream j;
    j << "{\"action\":[\n";
    bool first = true;
    for (const auto& [state,row] : action_)
        for (const auto& [sym,act] : row) {
            if (!first) j<<",\n"; first=false;
            j<<"  {\"state\":"<<state<<",\"symbol\":\""<<sym<<"\",\"action\":\""<<act.toString()<<"\"}";
        }
    j<<"\n],\"goto\":[\n"; first=true;
    for (const auto& [state,row] : goto_)
        for (const auto& [sym,dest] : row) {
            if (!first) j<<",\n"; first=false;
            j<<"  {\"state\":"<<state<<",\"symbol\":\""<<sym<<"\",\"goto\":"<<dest<<"}";
        }
    j<<"\n]}";
    return j.str();
}

void LALRTable::print() const {
    std::cout<<"=== Tabla LALR(1) ===\n";
    if (!conflicts_.empty()) std::cout<<"⚠️  "<<conflicts_.size()<<" conflicto(s)\n";
    else std::cout<<"✅ Sin conflictos\n";
}

} // namespace yapar
