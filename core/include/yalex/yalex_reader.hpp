#pragma once
#include <string>
#include <vector>
#include <map>
#include <unordered_map>
#include <optional>
#include <stdexcept>

namespace yalex {

struct LexRule {
    std::string pattern;
    std::string tokenName;
    int         priority;
    int         sourceLine = 0;   // línea en el .yalex donde se definió
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
    size_t      pos_      = 0;
    int         lineNum_  = 1;
    std::string sourceName_;
    std::string ruleName_;   // nombre de la regla activa (para detectar recursión OCamllex)

    // ── Parseo por formato ──────────────────────────────────
    void parseMacroBlock     (YAlexSpec& spec);
    void parseMacroDecl      (YAlexSpec& spec);
    void parseTopLevelDefs   (YAlexSpec& spec);
    void parseFlexPreamble   (YAlexSpec& spec);
    void parseFlexRules      (YAlexSpec& spec);
    void parseRuleHeader     (YAlexSpec& spec);
    void parseRuleSection    (YAlexSpec& spec);
    void skipPreambleBlock   ();

    // ── Parseo de reglas ────────────────────────────────────
    std::optional<LexRule> tryParseOneRule (int priority);
    std::string readPattern    ();
    std::string readTokenName  ();
    std::string extractTokenName(const std::string& actionContent);

    // ── Expansión de macros ─────────────────────────────────
    std::string expandBareMacros(const std::string& pattern,
                                  const std::unordered_map<std::string,std::string>& macros);

    // ── Utilidades ──────────────────────────────────────────
    void        skipWhitespaceAndComments();
    void        skipWS          ();   // solo espacios horizontales
    void        skipToEndOfLine ();
    std::string readWord        ();
    std::string readUntilEndOfLine();
    void        trimInPlace     (std::string& s);
    void        trimRight       (std::string& s);
    void        expect          (const std::string& word);
    bool        atEnd()  const  { return pos_ >= source_.size(); }
    char        current() const { return source_[pos_]; }
    [[noreturn]] void error(const std::string& msg);
};

} // namespace yalex
