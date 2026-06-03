// ============================================================
//  regex_engine.cpp
//  Implementación del parser de expresiones regulares
// ============================================================

#include "yalex/regex_engine.hpp"
#include <stack>
#include <sstream>
#include <algorithm>
#include <stdexcept>

namespace yalex {

// ── Constructor ───────────────────────────────────────────────
RegexParser::RegexParser(
    const std::unordered_map<std::string, std::string>& macros
) : macros_(macros) {}


// ════════════════════════════════════════════════════════════
//  parse() — orquesta los 4 pasos de transformación
// ════════════════════════════════════════════════════════════
std::vector<RegexToken> RegexParser::parse(const std::string& regex) {
    // Paso 1: expandir macros {nombre} y clases [a-z]
    std::string expanded = expandMacros(regex);

    // Paso 2: convertir el string a lista de tokens (infix)
    auto tokens = tokenize(expanded);

    // Paso 3: insertar operadores de concatenación explícitos
    auto withConcats = insertConcats(tokens);

    // Paso 4: convertir infix → postfix con Shunting-Yard
    return toPostfix(withConcats);
}


// ════════════════════════════════════════════════════════════
//  PASO 1: expandMacros
//  Busca patrones {nombre} y los reemplaza con la expresión
//  correspondiente del mapa de macros.
//  Ejemplo: "({letter})+" con macros["letter"] = "[a-z]"
//           → "([a-z])+"
// ════════════════════════════════════════════════════════════
std::string RegexParser::expandMacros(const std::string& regex) {
    std::string result = regex;

    // Iteramos hasta que no queden más macros que expandir
    // (las macros pueden referirse a otras macros)
    bool changed = true;
    int  iterations = 0;
    while (changed && iterations < 10) {
        changed = false;
        iterations++;

        std::string temp;
        size_t i = 0;
        while (i < result.size()) {
            if (result[i] == '{') {
                // Encontramos inicio de macro, buscamos el cierre
                size_t end = result.find('}', i);
                if (end == std::string::npos)
                    throw std::invalid_argument("Macro sin cerrar '}'");

                // Extraemos el nombre de la macro
                std::string name = result.substr(i + 1, end - i - 1);

                // Buscamos en el mapa
                auto it = macros_.find(name);
                if (it == macros_.end())
                    throw std::invalid_argument("Macro no definida: " + name);

                // Reemplazamos envolviendo en paréntesis para mantener precedencia
                temp += "(" + it->second + ")";
                i = end + 1;
                changed = true;
            } else {
                temp += result[i++];
            }
        }
        result = temp;
    }

    // Ahora expandimos clases de caracteres [...]
    std::string final_result;
    size_t i = 0;
    while (i < result.size()) {
        if (result[i] == '[') {
            size_t end = result.find(']', i);
            if (end == std::string::npos)
                throw std::invalid_argument("Clase de caracteres sin cerrar ']'");

            // Extraemos el contenido de la clase: "a-zA-Z0-9"
            std::string classContent = result.substr(i + 1, end - i - 1);
            final_result += expandCharClass(classContent);
            i = end + 1;
        } else {
            final_result += result[i++];
        }
    }

    return final_result;
}


// ════════════════════════════════════════════════════════════
//  expandCharClass
//  Convierte "[a-zA-Z0-9_]" → "(a|b|...|z|A|...|Z|0|...|9|_)"
//  Maneja:
//    - rangos: a-z, A-Z, 0-9
//    - literales sueltos: _, !, etc.
//    - negación: [^abc] → cualquier cosa excepto a, b, c
// ════════════════════════════════════════════════════════════
std::string RegexParser::expandCharClass(const std::string& classContent) {
    std::vector<char> chars;
    bool negate = false;
    size_t start = 0;

    // ¿Es una clase negada [^...]?
    if (!classContent.empty() && classContent[0] == '^') {
        negate = true;
        start = 1;
    }

    // Recorremos el contenido expandiendo rangos
    for (size_t i = start; i < classContent.size(); i++) {
        if (i + 2 < classContent.size() && classContent[i + 1] == '-') {
            // Es un rango: X-Y → agrega todos los chars de X a Y
            char from = classContent[i];
            char to   = classContent[i + 2];
            if (from > to)
                throw std::invalid_argument(
                    std::string("Rango inválido: ") + from + "-" + to
                );
            for (char c = from; c <= to; c++)
                chars.push_back(c);
            i += 2; // saltamos el '-' y el caracter final del rango
        } else {
            // Es un caracter literal dentro de la clase
            chars.push_back(classContent[i]);
        }
    }

    // Si es negación, necesitamos todos los chars ASCII imprimibles
    // excepto los especificados
    if (negate) {
        std::vector<char> allPrintable;
        for (char c = 32; c < 127; c++) allPrintable.push_back(c);

        std::vector<char> negated;
        for (char c : allPrintable) {
            if (std::find(chars.begin(), chars.end(), c) == chars.end())
                negated.push_back(c);
        }
        chars = negated;
    }

    if (chars.empty())
        throw std::invalid_argument("Clase de caracteres vacía");

    // Construimos la alternativa: (a|b|c|...)
    std::string result = "(";
    for (size_t i = 0; i < chars.size(); i++) {
        if (i > 0) result += "|";

        // Escapamos caracteres especiales de regex
        char c = chars[i];
        if (c == '(' || c == ')' || c == '*' || c == '+' ||
            c == '?' || c == '|' || c == '\\') {
            result += '\\';
        }
        result += c;
    }
    result += ")";
    return result;
}


// ════════════════════════════════════════════════════════════
//  PASO 2: tokenize
//  Convierte el string expandido en lista de RegexToken.
//  Maneja:
//    - Caracteres escapados: \n, \t, \\, \*, etc.
//    - Espacios dentro de comillas: ' '
//    - Operadores: | * + ? ( )
//    - El punto . como "cualquier caracter"
// ════════════════════════════════════════════════════════════
std::vector<RegexToken> RegexParser::tokenize(const std::string& regex) {
    std::vector<RegexToken> tokens;
    size_t i = 0;

    while (i < regex.size()) {
        char c = regex[i];

        if (c == '\\' && i + 1 < regex.size()) {
            // Caracter escapado: \n, \t, \r, o cualquier otro como literal
            char next = regex[++i];
            char literal;
            switch (next) {
                case 'n':  literal = '\n'; break;
                case 't':  literal = '\t'; break;
                case 'r':  literal = '\r'; break;
                default:   literal = next; break; // \* → '*' literal, etc.
            }
            tokens.emplace_back(RegexTokenType::LITERAL, literal);

        } else if (c == '\'' && i + 2 < regex.size() && regex[i + 2] == '\'') {
            // Caracter entre comillas simples: ' ' → espacio literal
            tokens.emplace_back(RegexTokenType::LITERAL, regex[i + 1]);
            i += 2; // saltamos el caracter y la comilla de cierre

        } else {
            // Mapeo de operadores y caracteres especiales
            switch (c) {
                case '|': tokens.emplace_back(RegexTokenType::UNION);       break;
                case '*': tokens.emplace_back(RegexTokenType::KLEENE_STAR); break;
                case '+': tokens.emplace_back(RegexTokenType::PLUS);        break;
                case '?': tokens.emplace_back(RegexTokenType::OPTIONAL);    break;
                case '(': tokens.emplace_back(RegexTokenType::LPAREN);      break;
                case ')': tokens.emplace_back(RegexTokenType::RPAREN);      break;
                case '.': tokens.emplace_back(RegexTokenType::ANY);         break;
                default:
                    // Cualquier otro caracter es un literal
                    tokens.emplace_back(RegexTokenType::LITERAL, c);
                    break;
            }
        }
        i++;
    }

    return tokens;
}


// ════════════════════════════════════════════════════════════
//  PASO 3: insertConcats
//  La concatenación en regex es implícita: "ab" significa a·b.
//  Para que Shunting-Yard funcione, la hacemos explícita
//  insertando tokens CONCAT donde corresponde.
//
//  Regla de inserción — insertar CONCAT entre token[i] y token[i+1] si:
//    token[i] es:    LITERAL, RPAREN, KLEENE_STAR, PLUS, OPTIONAL, ANY
//    token[i+1] es:  LITERAL, LPAREN, ANY
// ════════════════════════════════════════════════════════════
std::vector<RegexToken> RegexParser::insertConcats(
    const std::vector<RegexToken>& tokens
) {
    std::vector<RegexToken> result;

    // Tipos que pueden ir a la IZQUIERDA de una concatenación implícita
    auto isLeftOk = [](RegexTokenType t) {
        return t == RegexTokenType::LITERAL     ||
               t == RegexTokenType::RPAREN      ||
               t == RegexTokenType::KLEENE_STAR ||
               t == RegexTokenType::PLUS        ||
               t == RegexTokenType::OPTIONAL    ||
               t == RegexTokenType::ANY;
    };

    // Tipos que pueden ir a la DERECHA de una concatenación implícita
    auto isRightOk = [](RegexTokenType t) {
        return t == RegexTokenType::LITERAL ||
               t == RegexTokenType::LPAREN  ||
               t == RegexTokenType::ANY;
    };

    for (size_t i = 0; i < tokens.size(); i++) {
        result.push_back(tokens[i]);

        // Si hay un token siguiente y aplica la regla, insertamos CONCAT
        if (i + 1 < tokens.size() &&
            isLeftOk(tokens[i].type) &&
            isRightOk(tokens[i + 1].type)) {
            result.emplace_back(RegexTokenType::CONCAT);
        }
    }

    return result;
}


// ════════════════════════════════════════════════════════════
//  PASO 4: toPostfix — Algoritmo Shunting-Yard
//
//  Convierte la lista de tokens en notación infix a postfix.
//  El algoritmo usa una pila para operadores y produce una
//  cola de salida con el orden postfix correcto.
//
//  Precedencia (mayor número = mayor prioridad):
//    | (UNION)    → 1
//    . (CONCAT)   → 2
//    * + ? (unario) → 3
//
//  Ejemplo:
//    infix:   a · b | c · d *
//    postfix: a b · c d * · |
// ════════════════════════════════════════════════════════════
std::vector<RegexToken> RegexParser::toPostfix(
    const std::vector<RegexToken>& tokens
) {
    std::vector<RegexToken> output;  // cola de salida (postfix)
    std::stack<RegexToken>  opStack; // pila de operadores

    for (const auto& token : tokens) {
        switch (token.type) {

            // Los literales y ANY van directo a la salida
            case RegexTokenType::LITERAL:
            case RegexTokenType::ANY:
            case RegexTokenType::EPSILON:
                output.push_back(token);
                break;

            // Paréntesis izquierdo: siempre va a la pila
            case RegexTokenType::LPAREN:
                opStack.push(token);
                break;

            // Paréntesis derecho: vacía la pila hasta encontrar el LPAREN
            case RegexTokenType::RPAREN:
                while (!opStack.empty() &&
                       opStack.top().type != RegexTokenType::LPAREN) {
                    output.push_back(opStack.top());
                    opStack.pop();
                }
                if (opStack.empty())
                    throw std::invalid_argument("Paréntesis desbalanceados en regex");
                opStack.pop(); // descarta el LPAREN
                break;

            // Operadores: respeta la precedencia
            case RegexTokenType::UNION:
            case RegexTokenType::CONCAT:
            case RegexTokenType::KLEENE_STAR:
            case RegexTokenType::PLUS:
            case RegexTokenType::OPTIONAL:
                // Mientras la pila tenga operadores de mayor o igual precedencia
                // (y no sea LPAREN), los movemos a la salida
                while (!opStack.empty() &&
                       opStack.top().type != RegexTokenType::LPAREN &&
                       precedence(opStack.top().type) >= precedence(token.type)) {
                    output.push_back(opStack.top());
                    opStack.pop();
                }
                opStack.push(token);
                break;

            default:
                break;
        }
    }

    // Vaciamos los operadores restantes en la pila
    while (!opStack.empty()) {
        if (opStack.top().type == RegexTokenType::LPAREN)
            throw std::invalid_argument("Paréntesis desbalanceados en regex");
        output.push_back(opStack.top());
        opStack.pop();
    }

    return output;
}


// ── Helpers de precedencia ────────────────────────────────────
int RegexParser::precedence(RegexTokenType type) {
    switch (type) {
        case RegexTokenType::KLEENE_STAR:
        case RegexTokenType::PLUS:
        case RegexTokenType::OPTIONAL:
            return 3; // mayor precedencia: operadores unarios
        case RegexTokenType::CONCAT:
            return 2;
        case RegexTokenType::UNION:
            return 1; // menor precedencia
        default:
            return 0;
    }
}

bool RegexParser::isBinaryOp(RegexTokenType type) {
    return type == RegexTokenType::UNION ||
           type == RegexTokenType::CONCAT;
}

bool RegexParser::isUnaryOp(RegexTokenType type) {
    return type == RegexTokenType::KLEENE_STAR ||
           type == RegexTokenType::PLUS        ||
           type == RegexTokenType::OPTIONAL;
}

} // namespace yalex