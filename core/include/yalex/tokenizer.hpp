#pragma once
#include "yalex/yalex_reader.hpp"
#include "yalex/nfa.hpp"
#include "yalex/dfa.hpp"
#include <vector>
#include <string>

namespace yalex {

struct Token {
    std::string type;
    std::string lexeme;
    int         line;
    int         column;
    Token(std::string t, std::string l, int ln, int col)
        : type(std::move(t)), lexeme(std::move(l)), line(ln), column(col) {}
};

struct LexError {
    std::string message;
    int         line;
    int         column;
    char        badChar;
};

struct TokenizeResult {
    std::vector<Token>    tokens;
    std::vector<LexError> errors;
    bool                  success;
    std::string           dfaJSON;
};

class Tokenizer {
public:
    void loadSpec(const YAlexSpec& spec);
    void loadFromFile(const std::string& yalexPath);
    TokenizeResult tokenize(const std::string& input, bool skipWhitespace = true) const;
    const DFA& getDFA() const { return dfa_; }
    std::string toJSON() const;
private:
    DFA       dfa_;
    YAlexSpec spec_;
    bool      loaded_ = false;
};

} // namespace yalex
