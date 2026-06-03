#include "yapar/lalr_table.hpp"
#include "yapar/grammar.hpp"
#include <iostream>
#include <sstream>
#include <queue>

namespace yapar {

// Escapa una cadena para emitirla de forma segura dentro de un JSON.
static std::string jsonEscape(const std::string& s) {
    std::string r;
    for (char c : s) {
        switch (c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n";  break;
            case '\r': r += "\\r";  break;
            case '\t': r += "\\t";  break;
            default:   r += c;
        }
    }
    return r;
}

void LALRTable::setAction(int state, const std::string& sym, const Action& a) {
    auto it = action_[state].find(sym);
    if (it == action_[state].end()) { action_[state][sym] = a; return; }
    if (it->second.type == a.type && it->second.value == a.value) return;

    const Action& existing = it->second;
    const Action& incoming = a;

    bool isShiftReduce =
        (existing.type == ActionType::SHIFT  && incoming.type == ActionType::REDUCE) ||
        (existing.type == ActionType::REDUCE && incoming.type == ActionType::SHIFT);

    if (isShiftReduce && automaton_) {
        const Grammar& g = automaton_->getGrammar();
        const Action& shiftAct  = (existing.type == ActionType::SHIFT) ? existing : incoming;
        const Action& reduceAct = (existing.type == ActionType::REDUCE) ? existing : incoming;

        const PrecInfo* symPrec  = g.getPrecedence(sym);
        const PrecInfo* prodPrec = g.getProductionPrec(reduceAct.value);

        if (symPrec && prodPrec) {
            Conflict c;
            c.state = state; c.symbol = sym;
            c.existing = existing; c.incoming = incoming;
            c.resolvedByPrec = true;

            if (symPrec->level > prodPrec->level) {
                action_[state][sym] = shiftAct;
                c.description = "LALR Shift/Reduce resuelto en estado " + std::to_string(state) +
                                " '" + sym + "': shift gana (nivel " +
                                std::to_string(symPrec->level) + " > " +
                                std::to_string(prodPrec->level) + ")";
                c.resolution = "shift";
            } else if (symPrec->level < prodPrec->level) {
                action_[state][sym] = reduceAct;
                c.description = "LALR Shift/Reduce resuelto en estado " + std::to_string(state) +
                                " '" + sym + "': reduce gana (nivel " +
                                std::to_string(prodPrec->level) + " > " +
                                std::to_string(symPrec->level) + ")";
                c.resolution = "reduce";
            } else {
                if (symPrec->assoc == Associativity::LEFT) {
                    action_[state][sym] = reduceAct;
                    c.description = "LALR Shift/Reduce resuelto en estado " +
                                    std::to_string(state) + " '" + sym +
                                    "': reduce (left-assoc, nivel " +
                                    std::to_string(symPrec->level) + ")";
                    c.resolution = "reduce (left-assoc)";
                } else if (symPrec->assoc == Associativity::RIGHT) {
                    action_[state][sym] = shiftAct;
                    c.description = "LALR Shift/Reduce resuelto en estado " +
                                    std::to_string(state) + " '" + sym +
                                    "': shift (right-assoc, nivel " +
                                    std::to_string(symPrec->level) + ")";
                    c.resolution = "shift (right-assoc)";
                } else {
                    action_[state][sym] = Action::error();
                    c.description = "LALR en estado " + std::to_string(state) +
                                    " '" + sym + "': error (nonassoc)";
                    c.resolution = "error (nonassoc)";
                }
            }
            resolvedConflicts_.push_back(c);
            std::cerr << "[LALR prec] " << c.description << "\n";
            return;
        }
    }

    Conflict c;
    c.state = state; c.symbol = sym;
    c.existing = existing; c.incoming = incoming;
    c.resolvedByPrec = false;
    c.description = "LALR conflicto en estado " + std::to_string(state) +
                    " '" + sym + "': " + existing.toString() + " vs " + incoming.toString();
    conflicts_.push_back(c);
    if (a.type == ActionType::SHIFT) action_[state][sym] = a;
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

// ════════════════════════════════════════════════════════════
//  computeLookaheads() — lookaheads LALR(1) correctos
//
//  Método: punto fijo de closure LR(1) sobre los estados LR(0).
//  Esto equivale a construir el autómata LR(1) y fusionar estados
//  con el mismo núcleo LR(0) — que es exactamente la definición
//  de LALR(1), pero sin duplicar estados.
//
//  Para cada estado s:
//    1. Formamos los ítems LR(1) a partir de los lookaheads
//       acumulados:  { (item, la) : la ∈ LA[s][item] }.
//    2. Calculamos su closure LR(1) (genera lookaheads espontáneos).
//    3. Guardamos esos lookaheads de vuelta en LA[s][item].
//    4. Para cada ítem con el punto antes de X, propagamos el
//       lookahead al ítem avanzado en GOTO(s, X).
//  Repetimos hasta que no haya cambios.
// ════════════════════════════════════════════════════════════
void LALRTable::computeLookaheads() {
    const Grammar& g = automaton_->getGrammar();
    const auto& states = automaton_->getStates();

    lookaheads_.clear();

    // Semilla: el ítem inicial S' → • S en el estado 0 mira $.
    lookaheads_[0][{0, 0}].insert(END_MARKER);

    bool changed = true;
    while (changed) {
        changed = false;

        for (const auto& state : states) {
            int s = state.id;

            auto sit = lookaheads_.find(s);
            if (sit == lookaheads_.end()) continue;

            // 1. Construir el conjunto de ítems LR(1) semilla del estado
            std::set<LR1Item> seed;
            for (const auto& [item, las] : sit->second)
                for (const auto& la : las)
                    seed.insert({item, la});
            if (seed.empty()) continue;

            // 2. Closure LR(1): genera lookaheads espontáneos
            std::set<LR1Item> clos = closureLR1(seed);

            // 3 + 4. Guardar lookaheads y propagar por las transiciones
            for (const auto& it1 : clos) {
                const LR0Item& item = it1.base;

                // Guardar el lookahead generado para este ítem en este estado
                if (lookaheads_[s][item].insert(it1.lookahead).second)
                    changed = true;

                const Production& prod = g.getProductions()[item.prodId];
                if (item.dotPos < (int)prod.rhs.size()) {
                    const std::string& X = prod.rhs[item.dotPos].name;
                    auto destIt = state.transitions.find(X);
                    if (destIt != state.transitions.end()) {
                        int dest = destIt->second;
                        LR0Item adv{item.prodId, item.dotPos + 1};
                        // Propagar el lookahead al ítem avanzado en el destino
                        if (lookaheads_[dest][adv].insert(it1.lookahead).second)
                            changed = true;
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
    resolvedConflicts_.clear();
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
                // REDUCE — usamos los lookaheads LALR calculados
                if (prod.lhs.name == grammar.getStartSymbol().name && item.prodId == 0) {
                    setAction(s, END_MARKER, Action::accept());
                } else {
                    // Lookaheads específicos para este ítem en este estado
                    bool usedLA = false;
                    auto stateIt = lookaheads_.find(s);
                    if (stateIt != lookaheads_.end()) {
                        auto itemIt = stateIt->second.find(item);
                        if (itemIt != stateIt->second.end() && !itemIt->second.empty()) {
                            for (const auto& la : itemIt->second)
                                setAction(s, la, Action::reduce(item.prodId));
                            usedLA = true;
                        }
                    }
                    // Fallback defensivo (SLR) SOLO si no se calcularon lookaheads.
                    // Antes esto se ejecutaba siempre y volvía LALR idéntico a SLR.
                    if (!usedLA) {
                        for (const auto& a : grammar.getFollow(prod.lhs.name))
                            setAction(s, a, Action::reduce(item.prodId));
                    }
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
            j<<"  {\"state\":"<<state<<",\"symbol\":\""<<jsonEscape(sym)<<"\",\"action\":\""<<jsonEscape(act.toString())<<"\"}";
        }
    j<<"\n],\"goto\":[\n"; first=true;
    for (const auto& [state,row] : goto_)
        for (const auto& [sym,dest] : row) {
            if (!first) j<<",\n"; first=false;
            j<<"  {\"state\":"<<state<<",\"symbol\":\""<<jsonEscape(sym)<<"\",\"goto\":"<<dest<<"}";
        }
    j<<"\n],\"conflicts\":[";
    for (size_t i = 0; i < conflicts_.size(); i++) {
        if (i) j<<",";
        j<<"\n  {\"state\":"<<conflicts_[i].state
         <<",\"symbol\":\""<<jsonEscape(conflicts_[i].symbol)
         <<"\",\"description\":\""<<jsonEscape(conflicts_[i].description)<<"\"}";
    }
    j<<"\n],\"resolvedConflicts\":[";
    for (size_t i = 0; i < resolvedConflicts_.size(); i++) {
        if (i) j<<",";
        j<<"\n  {\"state\":"<<resolvedConflicts_[i].state
         <<",\"symbol\":\""<<jsonEscape(resolvedConflicts_[i].symbol)
         <<"\",\"description\":\""<<jsonEscape(resolvedConflicts_[i].description)
         <<"\",\"resolution\":\""<<jsonEscape(resolvedConflicts_[i].resolution)<<"\"}";
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
