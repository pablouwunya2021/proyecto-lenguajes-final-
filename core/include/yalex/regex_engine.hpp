#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace yalex {

enum class RegexTokenType {
    LITERAL, UNION, CONCAT, KLEENE_STAR, PLUS, OPTIONAL, LPAREN, RPAREN, ANY, EPSILON
};

struct RegexToken {
    RegexTokenType type;
    char           value;
    RegexToken(RegexTokenType t, char v = '\0') : type(t), value(v) {}
};

class RegexParser {
public:
    explicit RegexParser(const std::unordered_map<std::string,std::string>& macros = {});
    std::vector<RegexToken> parse(const std::string& regex);
private:
    std::unordered_map<std::string,std::string> macros_;
    std::string             expandMacros(const std::string& regex);
    std::string             expandCharClass(const std::string& charClass);
    std::vector<RegexToken> tokenize(const std::string& regex);
    std::vector<RegexToken> insertConcats(const std::vector<RegexToken>& tokens);
    std::vector<RegexToken> toPostfix(const std::vector<RegexToken>& tokens);
    int  precedence(RegexTokenType type);
    bool isBinaryOp(RegexTokenType type);
    bool isUnaryOp(RegexTokenType type);
};

} // namespace yalex
