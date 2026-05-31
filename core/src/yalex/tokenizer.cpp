#include "yalex/tokenizer.hpp"
#include "yalex/regex_engine.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace yalex {

void Tokenizer::loadFromFile(const std::string& yalexPath) {
    YAlexReader reader;
    loadSpec(reader.readFile(yalexPath));
}

void Tokenizer::loadSpec(const YAlexSpec& spec) {
    spec_ = spec; loaded_ = false;
    RegexParser regexParser(spec.macros);
    CombinedNFA combinedNFA;
    std::cout << "[YALex] Procesando " << spec.rules.size() << " reglas...\n";
    for (const auto& rule : spec.rules) {
        try {
            std::cout << "  [" << rule.priority << "] " << rule.tokenName << " = " << rule.pattern << "\n";
            auto postfix = regexParser.parse(rule.pattern);
            NFA nfa; nfa.build(postfix, rule.tokenName);
            combinedNFA.addRule(std::move(nfa), rule.tokenName);
        } catch (const std::exception& e) {
            throw std::runtime_error("Error en regla '" + rule.tokenName + "': " + e.what());
        }
    }
    std::cout << "[YALex] Combinando NFAs...\n";
    NFA combined = combinedNFA.combine();
    std::cout << "[YALex] Subset Construction...\n";
    dfa_.buildFromNFA(combined);
    std::cout << "  Sin minimizar: " << dfa_.getStates().size() << " estados\n";
    std::cout << "[YALex] Minimizando (Hopcroft)...\n";
    dfa_.minimize();
    std::cout << "  Minimizado: " << dfa_.getStates().size() << " estados\n";
    loaded_ = true;
    std::cout << "[YALex] Listo!\n";
}

TokenizeResult Tokenizer::tokenize(const std::string& input, bool skipWhitespace) const {
    if (!loaded_) throw std::runtime_error("Tokenizer no inicializado");
    TokenizeResult result; result.success = true;
    size_t pos = 0; int line = 1, col = 1;
    while (pos < input.size()) {
        auto match = dfa_.nextMatch(input, pos, line, col);
        if (!match.has_value()) {
            LexError err;
            err.badChar = input[pos]; err.line = line; err.column = col;
            err.message = std::string("Caracter inesperado '") + input[pos] + "'";
            result.errors.push_back(err); result.success = false;
            if (input[pos]=='\n') { line++; col=1; } else { col++; }
            pos++;
        } else {
            if (!skipWhitespace || match->tokenName != "WHITESPACE")
                result.tokens.emplace_back(match->tokenName, match->lexeme, match->line, match->column);
        }
    }
    result.tokens.emplace_back("EOF", "", line, col);
    result.dfaJSON = dfa_.toJSON();
    return result;
}

std::string Tokenizer::toJSON() const {
    std::ostringstream j;
    j << "{\n  \"dfa\": " << dfa_.toJSON() << ",\n  \"rules\": [\n";
    for (size_t i = 0; i < spec_.rules.size(); i++) {
        const auto& r = spec_.rules[i];
        j << "    {\"priority\":" << r.priority << ",\"token\":\"" << r.tokenName
          << "\",\"pattern\":\"" << r.pattern << "\"}";
        if (i+1 < spec_.rules.size()) j << ",";
        j << "\n";
    }
    j << "  ]\n}";
    return j.str();
}

} // namespace yalex
