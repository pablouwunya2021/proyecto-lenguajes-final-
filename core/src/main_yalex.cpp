#include "yalex/tokenizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readTextFile(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) throw std::runtime_error("No se pudo abrir: " + path);
    std::ostringstream ss; ss << f.rdbuf(); return ss.str();
}

std::string jsonEscape(const std::string& s) {
    std::string r;
    for (char c : s) {
        switch(c) {
            case '"':  r += "\\\""; break;
            case '\\': r += "\\\\"; break;
            case '\n': r += "\\n";  break;
            case '\r': r += "\\r";  break;
            case '\t': r += "\\t";  break;
            default:   r += c;
        }
    }
    return r;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: yalex_cli <archivo.yalex> <input.txt>\n";
        std::cerr << "  o: yalex_cli <archivo.yalex> --string \"texto\"\n";
        return 1;
    }
    std::string yalexPath = argv[1];
    std::string inputText;
    if (std::string(argv[2]) == "--string") {
        if (argc < 4) { std::cerr << "--string requiere argumento\n"; return 1; }
        inputText = argv[3];
    } else {
        try { inputText = readTextFile(argv[2]); }
        catch (const std::exception& e) {
            std::cout << "{\"success\":false,\"error\":\"" << jsonEscape(e.what()) << "\"}\n";
            return 1;
        }
    }
    try {
        yalex::Tokenizer tok;
        tok.loadFromFile(yalexPath);
        auto result = tok.tokenize(inputText);
        std::cout << "{\n  \"success\": " << (result.success?"true":"false") << ",\n";
        std::cout << "  \"tokens\": [\n";
        for (size_t i = 0; i < result.tokens.size(); i++) {
            const auto& t = result.tokens[i];
            std::cout << "    {\"type\":\"" << t.type << "\",\"lexeme\":\""
                      << jsonEscape(t.lexeme) << "\",\"line\":" << t.line
                      << ",\"col\":" << t.column << "}";
            if (i+1 < result.tokens.size()) std::cout << ",";
            std::cout << "\n";
        }
        std::cout << "  ],\n  \"errors\": [\n";
        for (size_t i = 0; i < result.errors.size(); i++) {
            const auto& e = result.errors[i];
            std::cout << "    {\"message\":\"" << jsonEscape(e.message)
                      << "\",\"line\":" << e.line << ",\"col\":" << e.column << "}";
            if (i+1 < result.errors.size()) std::cout << ",";
            std::cout << "\n";
        }
        // DFA en JSON (para tablas) y en DOT (para Graphviz)
        std::string dfaDot = tok.getDFA().toDOT("DFA Minimizado");
        std::cout << "  ],\n"
                  << "  \"dfa\": " << result.dfaJSON << ",\n"
                  << "  \"dfa_dot\": \"" << jsonEscape(dfaDot) << "\"\n}\n";
    } catch (const std::exception& e) {
        std::cout << "{\"success\":false,\"error\":\"" << jsonEscape(e.what()) << "\"}\n";
        return 1;
    }
    return 0;
}
