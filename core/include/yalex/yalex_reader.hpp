#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>

namespace yalex {

struct LexRule {
    std::string pattern;
    std::string tokenName;
    int         priority;
};

struct YAlexSpec {
    std::unordered_map<std::string, std::string> macros;
    std::vector<LexRule> rules;
    std::string sourceFile;
};

class YAlexReader {
public:
    YAlexSpec readFile(const std::string& filepath);
    YAlexSpec readString(const std::string& content,
                         const std::string& sourceName = "<string>");
private:
    std::string source_;
    size_t      pos_;
    int         lineNum_;
    std::string sourceName_;

    void        parseMacroBlock(YAlexSpec& spec);
    void        parseMacroDecl(YAlexSpec& spec);
    void        parseRuleSection(YAlexSpec& spec);
    LexRule     parseOneRule(int priority);
    void        skipWhitespaceAndComments();
    std::string readWord();
    std::string readUntilEndOfLine();
    std::string readPattern();
    std::string readTokenName();
    void        expect(const std::string& word);
    bool        atEnd() const { return pos_ >= source_.size(); }
    char        current() const { return source_[pos_]; }
    [[noreturn]] void error(const std::string& msg);
};

} // namespace yalex
