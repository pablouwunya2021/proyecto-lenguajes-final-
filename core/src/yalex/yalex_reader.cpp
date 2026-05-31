#include "yalex/yalex_reader.hpp"
#include <fstream>
#include <sstream>
#include <cctype>

namespace yalex {

YAlexSpec YAlexReader::readFile(const std::string& filepath) {
    std::ifstream file(filepath);
    if (!file.is_open()) throw std::runtime_error("No se pudo abrir: " + filepath);
    std::ostringstream ss; ss << file.rdbuf();
    return readString(ss.str(), filepath);
}

YAlexSpec YAlexReader::readString(const std::string& content, const std::string& sourceName) {
    source_ = content; pos_ = 0; lineNum_ = 1; sourceName_ = sourceName;
    YAlexSpec spec; spec.sourceFile = sourceName;
    skipWhitespaceAndComments();
    if (!atEnd() && current() == '{') { parseMacroBlock(spec); skipWhitespaceAndComments(); }
    if (!atEnd()) {
        expect("rule"); skipWhitespaceAndComments();
        readWord();     skipWhitespaceAndComments();
        if (atEnd() || current() != '=') error("Se esperaba '='");
        pos_++;
        parseRuleSection(spec);
    }
    if (spec.rules.empty()) error("No contiene reglas");
    return spec;
}

void YAlexReader::parseMacroBlock(YAlexSpec& spec) {
    pos_++; skipWhitespaceAndComments();
    while (!atEnd() && current() != '}') {
        std::string w = readWord();
        if (w == "let") parseMacroDecl(spec);
        else error("Se esperaba 'let', encontrado: " + w);
        skipWhitespaceAndComments();
    }
    if (atEnd()) error("Bloque de macros sin cerrar");
    pos_++;
}

void YAlexReader::parseMacroDecl(YAlexSpec& spec) {
    skipWhitespaceAndComments();
    std::string name = readWord();
    if (name.empty()) error("Nombre de macro esperado");
    skipWhitespaceAndComments();
    if (atEnd() || current() != '=') error("Se esperaba '=' en macro '" + name + "'");
    pos_++; skipWhitespaceAndComments();
    std::string pattern = readUntilEndOfLine();
    size_t s = pattern.find_first_not_of(" \t");
    size_t e = pattern.find_last_not_of(" \t\r");
    if (s == std::string::npos) error("Macro '" + name + "' vacia");
    spec.macros[name] = pattern.substr(s, e - s + 1);
}

void YAlexReader::parseRuleSection(YAlexSpec& spec) {
    skipWhitespaceAndComments();
    int priority = 0;
    if (!atEnd() && current() != '|') spec.rules.push_back(parseOneRule(priority++));
    skipWhitespaceAndComments();
    while (!atEnd() && current() == '|') {
        pos_++; skipWhitespaceAndComments();
        spec.rules.push_back(parseOneRule(priority++));
        skipWhitespaceAndComments();
    }
}

LexRule YAlexReader::parseOneRule(int priority) {
    LexRule r; r.priority = priority;
    r.pattern   = readPattern();
    r.tokenName = readTokenName();
    return r;
}

std::string YAlexReader::readPattern() {
    std::string pattern;
    int braceDepth = 0;
    while (!atEnd()) {
        char c = current();
        if (c == '{') {
            if (braceDepth == 0 && !pattern.empty()) {
                size_t look = pos_ + 1;
                while (look < source_.size() && source_[look] == ' ') look++;
                if (look < source_.size() && isupper(source_[look])) break;
            }
            braceDepth++; pattern += c; pos_++;
        } else if (c == '}') {
            if (braceDepth > 0) { braceDepth--; pattern += c; pos_++; }
            else break;
        } else if (c == '\n') {
            lineNum_++; pos_++;
            if (!pattern.empty()) {
                size_t last = pattern.find_last_not_of(" \t");
                if (last != std::string::npos) pattern = pattern.substr(0, last + 1);
                break;
            }
        } else { pattern += c; pos_++; }
    }
    size_t s = pattern.find_first_not_of(" \t");
    size_t e = pattern.find_last_not_of(" \t");
    if (s == std::string::npos) error("Patron vacio");
    return pattern.substr(s, e - s + 1);
}

std::string YAlexReader::readTokenName() {
    skipWhitespaceAndComments();
    if (atEnd() || current() != '{') error("Se esperaba '{' para el token");
    pos_++; skipWhitespaceAndComments();
    std::string name;
    while (!atEnd() && (isalnum(current()) || current() == '_')) { name += current(); pos_++; }
    if (name.empty()) error("Nombre de token vacio");
    skipWhitespaceAndComments();
    if (atEnd() || current() != '}') error("Se esperaba '}' despues de '" + name + "'");
    pos_++;
    return name;
}

void YAlexReader::skipWhitespaceAndComments() {
    while (!atEnd()) {
        char c = current();
        if (c==' '||c=='\t'||c=='\r') { pos_++; }
        else if (c=='\n') { pos_++; lineNum_++; }
        else if (c=='('&&pos_+1<source_.size()&&source_[pos_+1]=='*') {
            pos_+=2;
            while (!atEnd()) {
                if (current()=='*'&&pos_+1<source_.size()&&source_[pos_+1]==')') { pos_+=2; break; }
                if (current()=='\n') lineNum_++;
                pos_++;
            }
        } else if (c=='#') { while (!atEnd()&&current()!='\n') pos_++; }
        else break;
    }
}

std::string YAlexReader::readWord() {
    std::string w;
    while (!atEnd() && (isalnum(current()) || current()=='_')) { w += current(); pos_++; }
    return w;
}

std::string YAlexReader::readUntilEndOfLine() {
    std::string r;
    while (!atEnd() && current()!='\n' && current()!='\r') { r += current(); pos_++; }
    return r;
}

void YAlexReader::expect(const std::string& word) {
    skipWhitespaceAndComments();
    std::string found = readWord();
    if (found != word) error("Se esperaba '" + word + "', encontrado: '" + found + "'");
}

void YAlexReader::error(const std::string& msg) {
    throw std::runtime_error(sourceName_ + ":" + std::to_string(lineNum_) + ": " + msg);
}

} // namespace yalex
