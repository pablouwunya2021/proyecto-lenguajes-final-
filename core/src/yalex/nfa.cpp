#include "yalex/nfa.hpp"
#include <stack>
#include <iostream>
#include <stdexcept>
#include <algorithm>

namespace yalex {

int NFA::newState() {
    states_.emplace_back(nextStateId_);
    return nextStateId_++;
}

void NFA::addTransition(int from, int to, char c) {
    while ((int)states_.size() <= std::max(from, to))
        states_.emplace_back(nextStateId_++);
    states_[from].transitions[c].insert(to);
}

NFAFragment NFA::buildLiteral(char c) {
    int q0 = newState(), q1 = newState();
    addTransition(q0, q1, c);
    return NFAFragment(q0, {q1});
}
NFAFragment NFA::buildEpsilon() {
    int q0 = newState(), q1 = newState();
    addTransition(q0, q1, EPSILON_CHAR);
    return NFAFragment(q0, {q1});
}
NFAFragment NFA::buildAny() {
    int q0 = newState(), q1 = newState();
    for (char c = 32; c < 127; c++) addTransition(q0, q1, c);
    return NFAFragment(q0, {q1});
}
NFAFragment NFA::buildUnion(NFAFragment a, NFAFragment b) {
    int q_new = newState(), q_final = newState();
    addTransition(q_new, a.start, EPSILON_CHAR);
    addTransition(q_new, b.start, EPSILON_CHAR);
    for (int s : a.accepting) addTransition(s, q_final, EPSILON_CHAR);
    for (int s : b.accepting) addTransition(s, q_final, EPSILON_CHAR);
    return NFAFragment(q_new, {q_final});
}
NFAFragment NFA::buildConcat(NFAFragment a, NFAFragment b) {
    for (int s : a.accepting) addTransition(s, b.start, EPSILON_CHAR);
    return NFAFragment(a.start, b.accepting);
}
NFAFragment NFA::buildKleeneStar(NFAFragment a) {
    int q_new = newState(), q_final = newState();
    addTransition(q_new, a.start,  EPSILON_CHAR);
    addTransition(q_new, q_final,  EPSILON_CHAR);
    for (int s : a.accepting) {
        addTransition(s, a.start,  EPSILON_CHAR);
        addTransition(s, q_final,  EPSILON_CHAR);
    }
    return NFAFragment(q_new, {q_final});
}
NFAFragment NFA::buildPlus(NFAFragment a) {
    int q_final = newState();
    for (int s : a.accepting) {
        addTransition(s, a.start, EPSILON_CHAR);
        addTransition(s, q_final, EPSILON_CHAR);
    }
    return NFAFragment(a.start, {q_final});
}
NFAFragment NFA::buildOptional(NFAFragment a) {
    int q_new = newState(), q_final = newState();
    addTransition(q_new, a.start,  EPSILON_CHAR);
    addTransition(q_new, q_final,  EPSILON_CHAR);
    for (int s : a.accepting) addTransition(s, q_final, EPSILON_CHAR);
    return NFAFragment(q_new, {q_final});
}

void NFA::build(const std::vector<RegexToken>& postfix, const std::string& tokenName) {
    std::stack<NFAFragment> stack;
    for (const auto& token : postfix) {
        switch (token.type) {
            case RegexTokenType::LITERAL:     stack.push(buildLiteral(token.value)); break;
            case RegexTokenType::ANY:         stack.push(buildAny());                break;
            case RegexTokenType::EPSILON:     stack.push(buildEpsilon());            break;
            case RegexTokenType::KLEENE_STAR: { auto a=stack.top();stack.pop(); stack.push(buildKleeneStar(a)); break; }
            case RegexTokenType::PLUS:        { auto a=stack.top();stack.pop(); stack.push(buildPlus(a));       break; }
            case RegexTokenType::OPTIONAL:    { auto a=stack.top();stack.pop(); stack.push(buildOptional(a));   break; }
            case RegexTokenType::CONCAT: {
                auto b=stack.top();stack.pop(); auto a=stack.top();stack.pop();
                stack.push(buildConcat(a,b)); break;
            }
            case RegexTokenType::UNION: {
                auto b=stack.top();stack.pop(); auto a=stack.top();stack.pop();
                stack.push(buildUnion(a,b)); break;
            }
            default: break;
        }
    }
    if (stack.size() != 1) throw std::invalid_argument("Expresion regular mal formada");
    NFAFragment result = stack.top();
    startState_ = result.start;
    if (result.accepting.size() != 1) throw std::logic_error("NFA debe tener 1 estado de aceptacion");
    acceptingState_ = *result.accepting.begin();
    states_[acceptingState_].isAccepting = true;
    states_[acceptingState_].tokenName   = tokenName;
}

std::set<int> NFA::epsilonClosure(const std::set<int>& states) const {
    std::set<int> closure = states;
    std::stack<int> worklist;
    for (int s : states) worklist.push(s);
    while (!worklist.empty()) {
        int cur = worklist.top(); worklist.pop();
        if (cur < 0 || cur >= (int)states_.size()) continue;
        auto it = states_[cur].transitions.find(EPSILON_CHAR);
        if (it != states_[cur].transitions.end())
            for (int next : it->second)
                if (!closure.count(next)) { closure.insert(next); worklist.push(next); }
    }
    return closure;
}

std::set<int> NFA::move(const std::set<int>& states, char c) const {
    std::set<int> result;
    for (int s : states) {
        if (s < 0 || s >= (int)states_.size()) continue;
        auto it = states_[s].transitions.find(c);
        if (it != states_[s].transitions.end())
            result.insert(it->second.begin(), it->second.end());
    }
    return result;
}

void NFA::print() const {
    std::cout << "=== NFA (" << states_.size() << " estados) ===\n";
    std::cout << "Inicio: q" << startState_ << "\n";
    for (const auto& s : states_) {
        std::cout << "q" << s.id;
        if (s.isAccepting) std::cout << " [ACEPTA: " << s.tokenName << "]";
        std::cout << ":\n";
        for (const auto& [c, targets] : s.transitions) {
            std::cout << "  --" << (c==EPSILON_CHAR ? "e" : std::string(1,c)) << "--> { ";
            for (int t : targets) std::cout << "q" << t << " ";
            std::cout << "}\n";
        }
    }
}

// ── CombinedNFA ───────────────────────────────────────────────
void CombinedNFA::addRule(NFA nfa, const std::string& tokenName) {
    rules_.emplace_back(std::move(nfa), tokenName);
}

NFA CombinedNFA::combine() {
    if (rules_.empty()) throw std::runtime_error("CombinedNFA: sin reglas");
    NFA combined;
    int superStart = combined.newState(); // estado 0
    int offset = 1;
    for (auto& [nfa, tokenName] : rules_) {
        const auto& nfaStates = nfa.getStates();
        int nfaStart = nfa.getStartState();
        int maxNewId = offset + (int)nfaStates.size() - 1;
        while ((int)combined.states_.size() <= maxNewId)
            combined.states_.emplace_back(combined.nextStateId_++);
        for (const auto& state : nfaStates) {
            int newId = state.id + offset;
            combined.states_[newId].isAccepting = state.isAccepting;
            combined.states_[newId].tokenName   = state.tokenName;
            for (const auto& [c, targets] : state.transitions)
                for (int t : targets)
                    combined.states_[newId].transitions[c].insert(t + offset);
        }
        combined.states_[superStart].transitions[EPSILON_CHAR].insert(nfaStart + offset);
        offset += (int)nfaStates.size();
    }
    combined.startState_     = superStart;
    combined.acceptingState_ = -1;
    return combined;
}

} // namespace yalex
