#include "yapar/grammar.hpp"
#include "yapar/lr0_automaton.hpp"
#include "yapar/slr1_table.hpp"
#include "yapar/ll1_table.hpp"
#include "yapar/lalr_table.hpp"
#include "yapar/conflict_resolver.hpp"
#include "yapar/parser.hpp"
#include "yalex/tokenizer.hpp"
#include <iostream>
#include <fstream>
#include <sstream>

std::string readFile(const std::string& p) {
    std::ifstream f(p); if(!f.is_open()) throw std::runtime_error("No se pudo abrir: "+p);
    std::ostringstream ss; ss<<f.rdbuf(); return ss.str();
}

std::string escape(const std::string& s) {
    std::string r;
    for(char c:s){ if(c=='"')r+="\\\""; else if(c=='\\')r+="\\\\"; else if(c=='\n')r+="\\n"; else r+=c; }
    return r;
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Uso: yapar_cli <archivo.yapar> <input.txt>\n";
        std::cerr << "  o: yapar_cli <archivo.yapar> --string \"texto\" [archivo.yalex]\n";
        return 1;
    }

    std::string yaparPath = argv[1];
    std::string inputText;
    std::string yalexPath = "";

    if (std::string(argv[2]) == "--string") {
        if (argc < 4) { std::cerr << "--string requiere argumento\n"; return 1; }
        inputText = argv[3];
        if (argc >= 5) yalexPath = argv[4];
    } else {
        try { inputText = readFile(argv[2]); } catch(...) {}
        if (argc >= 4) yalexPath = argv[3];
    }

    try {
        // 1. Cargar gramática
        yapar::Grammar grammar;
        grammar.loadFromFile(yaparPath);
        std::cerr << "[YAPar] Gramática cargada: "
                  << grammar.getProductions().size() << " producciones\n";

        // 2. Construir autómata LR(0)
        yapar::LR0Automaton automaton;
        automaton.build(grammar);
        std::cerr << "[YAPar] Autómata LR(0): "
                  << automaton.getStates().size() << " estados\n";

        // 3. Construir tablas en paralelo
        yapar::SLR1Table slr;
        yapar::LL1Table  ll1;
        yapar::LALRTable lalr;

        std::thread t1([&](){ slr.build(automaton);  });
        std::thread t2([&](){ ll1.build(grammar);    });
        std::thread t3([&](){ lalr.build(automaton); });
        t1.join(); t2.join(); t3.join();

        std::cerr << "[YAPar] Tablas construidas\n";
        std::cerr << "  SLR(1)  conflictos: " << slr.getConflicts().size()  << "\n";
        std::cerr << "  LL(1)   conflictos: " << ll1.getConflicts().size()  << "\n";
        std::cerr << "  LALR(1) conflictos: " << lalr.getConflicts().size() << "\n";

        // 4. Resolver conflictos SLR con paralelismo
        yapar::ConflictResolver resolver;
        auto analyses = resolver.resolveAll(slr.getConflicts(), grammar);

        // 5. Tokenizar el input (si hay .yalex)
        std::vector<yalex::Token> tokens;
        if (!yalexPath.empty()) {
            yalex::Tokenizer tok;
            tok.loadFromFile(yalexPath);
            auto lexResult = tok.tokenize(inputText, /*skipWhitespace=*/true);
            tokens = lexResult.tokens;
        }

        // 6. Parsear (si hay tokens)
        yapar::ParseResult parseResult;
        if (!tokens.empty()) {
            yapar::Parser parser;
            // Usamos LALR(1): es estrictamente más potente que SLR(1)
            // (resuelve gramáticas LALR que SLR no puede) y aplica la
            // resolución por precedencia igual que SLR.
            parser.useTable(lalr, grammar);
            parseResult = parser.parse(tokens);
        }

        // 7. Salida JSON
        // Generar el DOT del autómata LR(0)
        std::string lr0Dot = automaton.toDOT("Automata LR(0)");

        std::cout << "{\n"
                  << "  \"success\": " << (parseResult.success||tokens.empty()?"true":"false") << ",\n"
                  << "  \"grammar\": " << grammar.toJSON() << ",\n"
                  << "  \"automaton\": " << automaton.toJSON() << ",\n"
                  << "  \"automaton_dot\": \"" << escape(lr0Dot) << "\",\n"
                  << "  \"slr\": " << slr.toJSON() << ",\n"
                  << "  \"ll1\": " << ll1.toJSON() << ",\n"
                  << "  \"lalr\": " << lalr.toJSON() << ",\n"
                  << "  \"conflicts\": " << resolver.toJSON(analyses);

        if (!tokens.empty() && parseResult.tree)
            std::cout << ",\n  \"tree\": " << parseResult.tree->toJSON();

        if (!parseResult.errors.empty()) {
            std::cout << ",\n  \"errors\": [";
            for (size_t i=0; i<parseResult.errors.size(); i++) {
                if(i) std::cout<<",";
                std::cout << "\"" << escape(parseResult.errors[i]) << "\"";
            }
            std::cout << "]";
        }
        std::cout << "\n}\n";

    } catch (const std::exception& e) {
        std::cout << "{\"success\":false,\"error\":\"" << escape(e.what()) << "\"}\n";
        return 1;
    }
    return 0;
}
