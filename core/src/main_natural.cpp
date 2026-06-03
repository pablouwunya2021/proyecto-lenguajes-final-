// ============================================================
//  main_natural.cpp — CLI para traducción Q'eqchi' → Español
//  Uso: ./natural_cli "texto en qeqchi"
//       ./natural_cli --file archivo.txt
// ============================================================
#include "natural/qeqchi_translator.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

std::string escape(const std::string& s) {
    std::string r;
    for (char c : s) {
        if (c == '"')  r += "\\\"";
        else if (c == '\\') r += "\\\\";
        else if (c == '\n') r += "\\n";
        else r += c;
    }
    return r;
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Uso: natural_cli \"texto q'eqchi'\"\n";
        std::cerr << "  o: natural_cli --file archivo.txt\n";
        return 1;
    }

    std::string inputText;

    if (std::string(argv[1]) == "--file") {
        if (argc < 3) { std::cerr << "--file requiere ruta\n"; return 1; }
        std::ifstream f(argv[2]);
        if (!f.is_open()) {
            std::cout << "{\"success\":false,\"error\":\"No se pudo abrir el archivo\"}\n";
            return 1;
        }
        std::ostringstream ss; ss << f.rdbuf();
        inputText = ss.str();
    } else {
        inputText = argv[1];
    }

    try {
        natural::QeqchiTranslator translator;
        auto result = translator.translate(inputText);

        std::cout << "{\n"
                  << "  \"success\": true,\n"
                  << "  \"translation\": " << translator.toJSON(result) << "\n"
                  << "}\n";
    } catch (const std::exception& e) {
        std::cout << "{\"success\":false,\"error\":\"" << escape(e.what()) << "\"}\n";
        return 1;
    }
    return 0;
}
