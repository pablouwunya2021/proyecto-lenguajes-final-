#pragma once
#include "yalex/nfa.hpp"
#include <vector>
#include <map>
#include <set>
#include <string>
#include <unordered_map>
#include <optional>

namespace yalex {

struct DFAState {
    int id;
    bool isAccepting;
    std::string tokenName;
    int priority;
    std::map<char, int> transitions;
    std::set<int> nfaStates;
    explicit DFAState(int id) : id(id), isAccepting(false), priority(0) {}
};

struct DFAMatch {
    bool        matched;
    std::string tokenName;
    std::string lexeme;
    int         line;
    int         column;
};

class DFA {
public:
    DFA() : startState_(0), deadState_(-1) {}

    void buildFromNFA(const NFA& nfa);
    void minimize();

    std::optional<DFAMatch> nextMatch(
        const std::string& input, size_t& pos, int& line, int& col
    ) const;

    int getStartState() const { return startState_; }
    const std::vector<DFAState>& getStates() const { return states_; }

    std::string toJSON() const;
    std::string toDOT(const std::string& title = "DFA") const;
    void        print() const;

private:
    std::vector<DFAState> states_;
    int startState_;
    int deadState_;

    std::string    setToKey(const std::set<int>& s) const;
    std::set<char> getAlphabet(const NFA& nfa) const;
    std::vector<std::set<int>> refine(
        const std::vector<std::set<int>>& partition, char c
    ) const;
};

} // namespace yalex
