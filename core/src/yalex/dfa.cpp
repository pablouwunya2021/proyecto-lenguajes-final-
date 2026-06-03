#include "yalex/dfa.hpp"
#include <stack>
#include <queue>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <climits>

namespace yalex {

std::string DFA::setToKey(const std::set<int>& s) const {
    std::ostringstream oss;
    bool first = true;
    for (int x : s) { if (!first) oss << ","; oss << x; first = false; }
    return oss.str();
}

std::set<char> DFA::getAlphabet(const NFA& nfa) const {
    std::set<char> alpha;
    for (const auto& state : nfa.getStates())
        for (const auto& [c, _] : state.transitions)
            if (c != EPSILON_CHAR) alpha.insert(c);
    return alpha;
}

void DFA::buildFromNFA(const NFA& nfa) {
    states_.clear();
    std::set<char> alphabet = getAlphabet(nfa);
    std::unordered_map<std::string, int> nfaSetToDFA;
    std::queue<std::set<int>> worklist;

    std::set<int> startSet = nfa.epsilonClosure({nfa.getStartState()});
    DFAState startDFA(0);
    startDFA.nfaStates = startSet;
    states_.push_back(startDFA);
    nfaSetToDFA[setToKey(startSet)] = 0;
    startState_ = 0;
    worklist.push(startSet);

    while (!worklist.empty()) {
        std::set<int> current = worklist.front(); worklist.pop();
        int curId = nfaSetToDFA[setToKey(current)];
        for (char c : alphabet) {
            std::set<int> moved   = nfa.move(current, c);
            if (moved.empty()) continue;
            std::set<int> nextSet = nfa.epsilonClosure(moved);
            if (nextSet.empty()) continue;
            std::string nextKey = setToKey(nextSet);
            if (!nfaSetToDFA.count(nextKey)) {
                int newId = (int)states_.size();
                DFAState ns(newId); ns.nfaStates = nextSet;
                states_.push_back(ns);
                nfaSetToDFA[nextKey] = newId;
                worklist.push(nextSet);
            }
            states_[curId].transitions[c] = nfaSetToDFA[nextKey];
        }
    }

    const auto& nfaStates = nfa.getStates();
    for (auto& dfaState : states_) {
        std::string bestToken;
        int bestPriority = INT_MAX;
        for (int nfaId : dfaState.nfaStates) {
            if (nfaId < (int)nfaStates.size() && nfaStates[nfaId].isAccepting) {
                if (nfaId < bestPriority) {
                    bestPriority = nfaId;
                    bestToken    = nfaStates[nfaId].tokenName;
                }
            }
        }
        if (!bestToken.empty()) {
            dfaState.isAccepting = true;
            dfaState.tokenName   = bestToken;
            dfaState.priority    = bestPriority;
        }
    }
}

void DFA::minimize() {
    if (states_.empty()) return;
    std::map<std::string, std::set<int>> groupMap;
    for (const auto& s : states_) {
        std::string key = s.isAccepting ? ("A_" + s.tokenName) : "NA";
        groupMap[key].insert(s.id);
    }
    std::vector<std::set<int>> partition;
    for (auto& [k, g] : groupMap) partition.push_back(g);

    std::set<char> alphabet;
    for (const auto& s : states_)
        for (const auto& [c, _] : s.transitions) alphabet.insert(c);

    bool changed = true;
    while (changed) {
        changed = false;
        std::vector<std::set<int>> newPartition;
        for (const auto& group : partition) {
            std::vector<std::set<int>> refined = {group};
            for (char c : alphabet) {
                std::vector<std::set<int>> nextRefined;
                for (const auto& sub : refined) {
                    std::map<int, std::set<int>> buckets;
                    for (int sid : sub) {
                        int dest = -1;
                        auto it = states_[sid].transitions.find(c);
                        if (it != states_[sid].transitions.end()) {
                            int ds = it->second;
                            for (int pi = 0; pi < (int)partition.size(); pi++)
                                if (partition[pi].count(ds)) { dest = pi; break; }
                        }
                        buckets[dest].insert(sid);
                    }
                    for (auto& [k, b] : buckets) nextRefined.push_back(b);
                }
                refined = nextRefined;
            }
            if (refined.size() > 1) changed = true;
            for (auto& g : refined) newPartition.push_back(g);
        }
        partition = newPartition;
    }

    std::map<int,int> oldToNew;
    for (int gi = 0; gi < (int)partition.size(); gi++)
        for (int sid : partition[gi]) oldToNew[sid] = gi;

    std::vector<DFAState> newStates;
    for (int gi = 0; gi < (int)partition.size(); gi++) {
        int rep = *partition[gi].begin();
        DFAState ns(gi);
        ns.isAccepting = states_[rep].isAccepting;
        ns.tokenName   = states_[rep].tokenName;
        ns.priority    = states_[rep].priority;
        ns.nfaStates   = states_[rep].nfaStates;
        for (const auto& [c, dest] : states_[rep].transitions)
            ns.transitions[c] = oldToNew[dest];
        newStates.push_back(ns);
    }
    startState_ = oldToNew[startState_];
    states_     = newStates;
}

std::optional<DFAMatch> DFA::nextMatch(
    const std::string& input, size_t& pos, int& line, int& col
) const {
    if (pos >= input.size()) return std::nullopt;
    int    cur = startState_;
    size_t i   = pos;
    int    lastAcceptState = -1;
    size_t lastAcceptPos   = pos;
    int    lastAcceptLine  = line, lastAcceptCol = col;
    int    curLine = line, curCol = col;

    while (i < input.size()) {
        auto it = states_[cur].transitions.find(input[i]);
        if (it == states_[cur].transitions.end()) break;
        cur = it->second; i++;
        if (input[i-1] == '\n') { curLine++; curCol = 1; } else { curCol++; }
        if (states_[cur].isAccepting) {
            lastAcceptState = cur;
            lastAcceptPos   = i;
            lastAcceptLine  = curLine;
            lastAcceptCol   = curCol;
        }
    }
    if (lastAcceptState == -1) return std::nullopt;

    DFAMatch match;
    match.matched   = true;
    match.tokenName = states_[lastAcceptState].tokenName;
    match.lexeme    = input.substr(pos, lastAcceptPos - pos);
    match.line      = line;
    match.column    = col;
    pos  = lastAcceptPos;
    line = lastAcceptLine;
    col  = lastAcceptCol;
    return match;
}

// ════════════════════════════════════════════════════════════
//  toDOT — genera un grafo en formato Graphviz DOT
//
//  Convenciones visuales formales:
//  - Estado inicial:    flecha entrante desde un nodo invisible
//  - Estados de aceptación: doble círculo (doublecircle)
//  - Estados normales:  círculo simple
//  - Transiciones con múltiples chars al mismo destino se agrupan
//  - Layout de izquierda a derecha (rankdir=LR)
// ════════════════════════════════════════════════════════════
std::string DFA::toDOT(const std::string& title) const {
    std::ostringstream d;

    d << "digraph DFA {\n";
    d << "  rankdir=LR;\n";
    d << "  bgcolor=\"transparent\";\n";
    d << "  fontname=\"Courier New\";\n";
    if (!title.empty())
        d << "  label=\"" << title << "\";\n"
          << "  labelloc=t;\n"
          << "  fontsize=14;\n"
          << "  fontcolor=\"#cccccc\";\n";

    // Nodo invisible para la flecha de estado inicial
    d << "  __start [shape=point, style=invis, width=0.1];\n";

    // Definir estados
    for (const auto& s : states_) {
        std::string shape = s.isAccepting ? "doublecircle" : "circle";
        std::string label = "q" + std::to_string(s.id);
        if (s.isAccepting && !s.tokenName.empty())
            label += "\\n[" + s.tokenName + "]";

        d << "  q" << s.id
          << " [shape=" << shape
          << ", label=\"" << label << "\""
          << ", fontname=\"Courier New\""
          << ", fontsize=10"
          << ", style=filled"
          << ", fillcolor=" << (s.isAccepting ? "\"#1a3d1a\"" : "\"#0d1a0d\"")
          << ", color=" << (s.isAccepting ? "\"#00ff41\"" : "\"#1a5c1a\"")
          << ", fontcolor=\"#00ff41\""
          << "];\n";
    }

    // Flecha al estado inicial
    d << "  __start -> q" << startState_
      << " [color=\"#00ff41\", penwidth=2];\n";

    // Agrupar transiciones paralelas (mismo origen y destino, distintos chars)
    std::map<std::pair<int,int>, std::vector<char>> grouped;
    for (const auto& s : states_)
        for (const auto& [c, dest] : s.transitions)
            grouped[{s.id, dest}].push_back(c);

    for (const auto& [key, chars] : grouped) {
        auto [from, to] = key;

        // Construir etiqueta agrupada (máx 6 chars, luego "...")
        std::string lbl;
        size_t maxShow = std::min(chars.size(), (size_t)6);
        for (size_t i = 0; i < maxShow; i++) {
            if (i > 0) lbl += "|";
            char c = chars[i];
            if      (c == '"')  lbl += "\\\"";
            else if (c == '\\') lbl += "\\\\";
            else if (c == '\n') lbl += "\\\\n";
            else if (c == '\r') lbl += "\\\\r";
            else if (c == '\t') lbl += "\\\\t";
            else if ((unsigned char)c < 32) lbl += "?";
            else lbl += c;
        }
        if (chars.size() > 6) lbl += "|...";

        d << "  q" << from << " -> q" << to
          << " [label=\"" << lbl << "\""
          << ", fontname=\"Courier New\""
          << ", fontsize=9"
          << ", color=\"#1a8c1a\""
          << ", fontcolor=\"#44cc44\""
          << "];\n";
    }

    d << "}\n";
    return d.str();
}

std::string DFA::toJSON() const {
    std::ostringstream j;
    j << "{\n  \"states\": [\n";
    for (size_t i = 0; i < states_.size(); i++) {
        const auto& s = states_[i];
        j << "    {\"id\":" << s.id << ",\"accepting\":"
          << (s.isAccepting?"true":"false") << ",\"token\":\"" << s.tokenName << "\"}";
        if (i+1 < states_.size()) j << ",";
        j << "\n";
    }
    j << "  ],\n  \"transitions\": [\n";
    bool first = true;
    for (const auto& s : states_) {
        for (const auto& [c, dest] : s.transitions) {
            if (!first) j << ",\n"; first = false;
            std::string lbl;
            if      (c == '"')  lbl = "\\\"";
            else if (c == '\\') lbl = "\\\\";
            else if (c == '\n') lbl = "\\n";
            else if (c == '\r') lbl = "\\r";
            else if (c == '\t') lbl = "\\t";
            else if ((unsigned char)c < 32) lbl = "?"; // otros control chars
            else lbl = std::string(1, c);
            j << "    {\"from\":" << s.id << ",\"to\":" << dest << ",\"label\":\"" << lbl << "\"}";
        }
    }
    j << "\n  ],\n  \"start\": " << startState_ << "\n}";
    return j.str();
}

void DFA::print() const {
    std::cout << "=== DFA (" << states_.size() << " estados) ===\nInicio: D" << startState_ << "\n";
    for (const auto& s : states_) {
        std::cout << "D" << s.id;
        if (s.isAccepting) std::cout << " [ACEPTA: " << s.tokenName << "]";
        std::cout << ":\n";
        for (const auto& [c, dest] : s.transitions)
            std::cout << "  --'" << c << "'--> D" << dest << "\n";
    }
}

} // namespace yalex
