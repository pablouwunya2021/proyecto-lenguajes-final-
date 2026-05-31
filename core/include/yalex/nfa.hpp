#pragma once
#include "yalex/regex_engine.hpp"
#include <vector>
#include <set>
#include <map>
#include <memory>
#include <string>

namespace yalex {

constexpr char EPSILON_CHAR = '\0';

struct NFAState {
    int id;
    bool isAccepting;
    std::string tokenName;
    std::map<char, std::set<int>> transitions;
    explicit NFAState(int id) : id(id), isAccepting(false) {}
};

struct NFAFragment {
    int           start;
    std::set<int> accepting;
    NFAFragment(int s, std::set<int> acc) : start(s), accepting(std::move(acc)) {}
};

class NFA {
public:
    NFA() : nextStateId_(0), startState_(0), acceptingState_(-1) {}

    void build(const std::vector<RegexToken>& postfix, const std::string& tokenName);

    std::set<int> epsilonClosure(const std::set<int>& states) const;
    std::set<int> move(const std::set<int>& states, char c) const;

    int getStartState()     const { return startState_; }
    int getAcceptingState() const { return acceptingState_; }
    const std::vector<NFAState>& getStates() const { return states_; }

    void print() const;

    // CombinedNFA necesita acceso directo
    friend class CombinedNFA;

private:
    std::vector<NFAState> states_;
    int nextStateId_;
    int startState_;
    int acceptingState_;

    int         newState();
    void        addTransition(int from, int to, char c);

    NFAFragment buildLiteral(char c);
    NFAFragment buildEpsilon();
    NFAFragment buildAny();
    NFAFragment buildUnion(NFAFragment a, NFAFragment b);
    NFAFragment buildConcat(NFAFragment a, NFAFragment b);
    NFAFragment buildKleeneStar(NFAFragment a);
    NFAFragment buildPlus(NFAFragment a);
    NFAFragment buildOptional(NFAFragment a);
};

class CombinedNFA {
public:
    void addRule(NFA nfa, const std::string& tokenName);
    NFA  combine();
private:
    std::vector<std::pair<NFA, std::string>> rules_;
};

} // namespace yalex
