#include "yalex/regex_engine.hpp"
#include <stack>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace yalex {

RegexParser::RegexParser(const std::unordered_map<std::string,std::string>& macros)
    : macros_(macros) {}

std::vector<RegexToken> RegexParser::parse(const std::string& regex) {
    std::string expanded   = expandMacros(regex);
    auto        tokens     = tokenize(expanded);
    auto        withConcat = insertConcats(tokens);
    return toPostfix(withConcat);
}

std::string RegexParser::expandMacros(const std::string& regex) {
    std::string result = regex;
    bool changed = true;
    int  iters   = 0;
    while (changed && iters < 10) {
        changed = false; iters++;
        std::string temp;
        size_t i = 0;
        while (i < result.size()) {
            if (result[i] == '{') {
                size_t end = result.find('}', i);
                if (end == std::string::npos) throw std::invalid_argument("Macro sin cerrar '}'");
                std::string name = result.substr(i+1, end-i-1);
                auto it = macros_.find(name);
                if (it == macros_.end()) throw std::invalid_argument("Macro no definida: " + name);
                temp += "(" + it->second + ")";
                i = end + 1; changed = true;
            } else { temp += result[i++]; }
        }
        result = temp;
    }
    // Expandir clases [...]
    std::string final_result;
    size_t i = 0;
    while (i < result.size()) {
        if (result[i] == '[') {
            size_t end = result.find(']', i);
            if (end == std::string::npos) throw std::invalid_argument("Clase sin cerrar ']'");
            final_result += expandCharClass(result.substr(i+1, end-i-1));
            i = end + 1;
        } else { final_result += result[i++]; }
    }
    return final_result;
}

std::string RegexParser::expandCharClass(const std::string& cls) {
    std::vector<char> chars;
    bool   negate = false;
    size_t i      = 0;
    if (!cls.empty() && cls[0] == '^') { negate = true; i = 1; }

    // Helper lambda: lee UN caracter desde la posición actual,
    // manejando notación 'x' y secuencias de escape '\t' '\n' '\r'
    // Avanza i hasta el siguiente elemento
    auto readOneChar = [&](size_t& pos) -> char {
        // Saltar espacios separadores entre elementos
        while (pos < cls.size() && cls[pos] == ' ') pos++;
        if (pos >= cls.size()) return '\0';

        if (cls[pos] == '\'') {
            // Notación 'x' o '\t' — char entre comillas simples
            pos++; // saltamos comilla de apertura
            char c;
            if (pos < cls.size() && cls[pos] == '\\' && pos+1 < cls.size()) {
                // Secuencia de escape: '\t', '\n', '\r', '\\'
                pos++; // saltamos la barra
                switch (cls[pos]) {
                    case 't':  c = '\t'; break;
                    case 'n':  c = '\n'; break;
                    case 'r':  c = '\r'; break;
                    default:   c = cls[pos]; break;
                }
                pos++;
            } else {
                c = cls[pos++];
            }
            if (pos < cls.size() && cls[pos] == '\'') pos++; // comilla de cierre
            return c;
        } else if (cls[pos] == '\\' && pos+1 < cls.size()) {
            // Secuencia de escape sin comillas: \t, \n, \r
            pos++;
            char c;
            switch (cls[pos]) {
                case 't':  c = '\t'; break;
                case 'n':  c = '\n'; break;
                case 'r':  c = '\r'; break;
                default:   c = cls[pos]; break;
            }
            pos++;
            return c;
        } else {
            return cls[pos++];
        }
    };

    while (i < cls.size()) {
        // Saltamos espacios sueltos entre elementos
        while (i < cls.size() && cls[i] == ' ') i++;
        if (i >= cls.size()) break;

        char from = readOneChar(i);

        // ¿Es un rango from-to?
        // Saltamos espacios y verificamos si sigue un '-'
        size_t saved = i;
        while (i < cls.size() && cls[i] == ' ') i++;
        if (i < cls.size() && cls[i] == '-') {
            i++; // saltamos el '-'
            char to = readOneChar(i);
            if (from > to) throw std::invalid_argument(
                std::string("Rango invalido: ") + from + "-" + to);
            for (char c = from; c <= to; c++) chars.push_back(c);
        } else {
            i = saved; // no era rango, restauramos
            chars.push_back(from);
        }
    }

    if (negate) {
        std::vector<char> neg;
        for (char c = 32; c < 127; c++)
            if (std::find(chars.begin(), chars.end(), c) == chars.end())
                neg.push_back(c);
        chars = neg;
    }
    if (chars.empty()) throw std::invalid_argument("Clase de caracteres vacia");

    std::string r = "(";
    for (size_t i = 0; i < chars.size(); i++) {
        if (i > 0) r += "|";
        char c = chars[i];
        if (c=='('||c==')'||c=='*'||c=='+'||c=='?'||c=='|'||c=='\\') r += '\\';
        r += c;
    }
    return r + ")";
}

std::vector<RegexToken> RegexParser::tokenize(const std::string& regex) {
    std::vector<RegexToken> tokens;
    size_t i = 0;
    while (i < regex.size()) {
        char c = regex[i];
        if (c == '\\' && i+1 < regex.size()) {
            char next = regex[++i];
            char lit = (next=='n') ? '\n' : (next=='t') ? '\t' : (next=='r') ? '\r' : next;
            tokens.emplace_back(RegexTokenType::LITERAL, lit);
        } else if (c == '\'' && i+2 < regex.size() && regex[i+2] == '\'') {
            // Caracter entre comillas simples: ' ' → espacio literal
            tokens.emplace_back(RegexTokenType::LITERAL, regex[i+1]);
            i += 2;
        } else if (c == '"') {
            // String entre comillas dobles: "if" → cada char es un LITERAL
            // Los operadores dentro de "" se tratan como caracteres normales
            i++; // saltamos la comilla de apertura
            while (i < regex.size() && regex[i] != '"') {
                tokens.emplace_back(RegexTokenType::LITERAL, regex[i]);
                i++;
            }
            // i apunta a la comilla de cierre, el i++ del loop la saltará
        } else {
            switch (c) {
                case '|': tokens.emplace_back(RegexTokenType::UNION);       break;
                case '*': tokens.emplace_back(RegexTokenType::KLEENE_STAR); break;
                case '+': tokens.emplace_back(RegexTokenType::PLUS);        break;
                case '?': tokens.emplace_back(RegexTokenType::OPTIONAL);    break;
                case '(': tokens.emplace_back(RegexTokenType::LPAREN);      break;
                case ')': tokens.emplace_back(RegexTokenType::RPAREN);      break;
                case '.': tokens.emplace_back(RegexTokenType::ANY);         break;
                default:  tokens.emplace_back(RegexTokenType::LITERAL, c);  break;
            }
        }
        i++;
    }
    return tokens;
}

std::vector<RegexToken> RegexParser::insertConcats(const std::vector<RegexToken>& tokens) {
    std::vector<RegexToken> result;
    auto isLeft = [](RegexTokenType t) {
        return t==RegexTokenType::LITERAL || t==RegexTokenType::RPAREN ||
               t==RegexTokenType::KLEENE_STAR || t==RegexTokenType::PLUS ||
               t==RegexTokenType::OPTIONAL || t==RegexTokenType::ANY;
    };
    auto isRight = [](RegexTokenType t) {
        return t==RegexTokenType::LITERAL || t==RegexTokenType::LPAREN || t==RegexTokenType::ANY;
    };
    for (size_t i = 0; i < tokens.size(); i++) {
        result.push_back(tokens[i]);
        if (i+1 < tokens.size() && isLeft(tokens[i].type) && isRight(tokens[i+1].type))
            result.emplace_back(RegexTokenType::CONCAT);
    }
    return result;
}

std::vector<RegexToken> RegexParser::toPostfix(const std::vector<RegexToken>& tokens) {
    std::vector<RegexToken> output;
    std::stack<RegexToken>  opStack;
    for (const auto& token : tokens) {
        switch (token.type) {
            case RegexTokenType::LITERAL:
            case RegexTokenType::ANY:
            case RegexTokenType::EPSILON:
                output.push_back(token); break;
            case RegexTokenType::LPAREN:
                opStack.push(token); break;
            case RegexTokenType::RPAREN:
                while (!opStack.empty() && opStack.top().type != RegexTokenType::LPAREN) {
                    output.push_back(opStack.top()); opStack.pop();
                }
                if (opStack.empty()) throw std::invalid_argument("Parentesis desbalanceados");
                opStack.pop(); break;
            default:
                while (!opStack.empty() &&
                       opStack.top().type != RegexTokenType::LPAREN &&
                       precedence(opStack.top().type) >= precedence(token.type)) {
                    output.push_back(opStack.top()); opStack.pop();
                }
                opStack.push(token); break;
        }
    }
    while (!opStack.empty()) {
        if (opStack.top().type == RegexTokenType::LPAREN)
            throw std::invalid_argument("Parentesis desbalanceados");
        output.push_back(opStack.top()); opStack.pop();
    }
    return output;
}

int RegexParser::precedence(RegexTokenType type) {
    switch (type) {
        case RegexTokenType::KLEENE_STAR:
        case RegexTokenType::PLUS:
        case RegexTokenType::OPTIONAL: return 3;
        case RegexTokenType::CONCAT:   return 2;
        case RegexTokenType::UNION:    return 1;
        default: return 0;
    }
}
bool RegexParser::isBinaryOp(RegexTokenType t) {
    return t==RegexTokenType::UNION || t==RegexTokenType::CONCAT;
}
bool RegexParser::isUnaryOp(RegexTokenType t) {
    return t==RegexTokenType::KLEENE_STAR || t==RegexTokenType::PLUS || t==RegexTokenType::OPTIONAL;
}

} // namespace yalex
