// ============================================================
//  tokenizer.cpp
//  Implementación del Tokenizador — une todo el pipeline
// ============================================================

#include "yalex/tokenizer.hpp"
#include "yalex/regex_engine.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace yalex {

// ════════════════════════════════════════════════════════════
//  loadFromFile() — atajo conveniente
// ════════════════════════════════════════════════════════════
void Tokenizer::loadFromFile(const std::string& yalexPath) {
    YAlexReader reader;
    YAlexSpec   spec = reader.readFile(yalexPath);
    loadSpec(spec);
}

// ════════════════════════════════════════════════════════════
//  loadSpec() — construye el DFA desde la especificación
//
//  Este es el método más importante del Tokenizer.
//  Orquesta todo el pipeline:
//    spec → RegexParser → NFA (por regla) → CombinedNFA → DFA
// ════════════════════════════════════════════════════════════
void Tokenizer::loadSpec(const YAlexSpec& spec) {
    spec_   = spec;
    loaded_ = false;

    // Creamos el parser de regex con las macros del .yalex
    // Así cuando encuentre {letter} en un patrón, lo expande
    RegexParser regexParser(spec.macros);

    // El CombinedNFA une todos los NFAs individuales
    CombinedNFA combinedNFA;

    std::cout << "[YALex] Procesando " << spec.rules.size()
              << " reglas...\n";

    // ── Por cada regla del .yalex ─────────────────────────────
    for (const auto& rule : spec.rules) {
        try {
            std::cout << "  Regla [" << rule.priority << "] "
                      << rule.tokenName << " = " << rule.pattern << "\n";

            // Paso 1: convertir el patrón a postfix
            auto postfix = regexParser.parse(rule.pattern);

            // Paso 2: construir el NFA para esta regla
            NFA nfa;
            nfa.build(postfix, rule.tokenName);

            // Paso 3: agregar al NFA combinado
            combinedNFA.addRule(std::move(nfa), rule.tokenName);

        } catch (const std::exception& e) {
            throw std::runtime_error(
                "Error en regla '" + rule.tokenName +
                "' (patrón: " + rule.pattern + "): " + e.what()
            );
        }
    }

    // Paso 4: combinar todos los NFAs en uno solo
    std::cout << "[YALex] Combinando NFAs...\n";
    NFA combined = combinedNFA.combine();

    // Paso 5: Subset Construction NFA → DFA
    std::cout << "[YALex] Construyendo DFA (Subset Construction)...\n";
    dfa_.buildFromNFA(combined);
    std::cout << "  DFA sin minimizar: " << dfa_.getStates().size()
              << " estados\n";

    // Paso 6: Minimizar el DFA (Hopcroft)
    std::cout << "[YALex] Minimizando DFA (Hopcroft)...\n";
    dfa_.minimize();
    std::cout << "  DFA minimizado: " << dfa_.getStates().size()
              << " estados\n";

    loaded_ = true;
    std::cout << "[YALex] ¡Listo!\n";
}

// ════════════════════════════════════════════════════════════
//  tokenize() — escanea el input y produce la lista de tokens
//
//  Principios de funcionamiento:
//  1. Maximal Munch: siempre acepta el lexema más largo
//  2. Prioridad: si dos reglas hacen match igual de largo,
//     gana la que aparece primero en el .yalex
//  3. Error Recovery: ante un error léxico, avanza 1 caracter
//     y continúa (para reportar todos los errores posibles)
// ════════════════════════════════════════════════════════════
TokenizeResult Tokenizer::tokenize(const std::string& input,
                                    bool skipWhitespace) const {
    if (!loaded_)
        throw std::runtime_error(
            "Tokenizer no inicializado. Llama loadSpec() primero."
        );

    TokenizeResult result;
    result.success = true;

    size_t pos  = 0;   // Posición actual en el input
    int    line = 1;   // Línea actual
    int    col  = 1;   // Columna actual

    while (pos < input.size()) {
        // Intentamos hacer match desde la posición actual
        auto match = dfa_.nextMatch(input, pos, line, col);

        if (!match.has_value()) {
            // ── Error léxico ──────────────────────────────────
            // No encontramos ningún token válido en esta posición.
            // Reportamos el error y avanzamos 1 caracter (recovery).
            LexError err;
            err.badChar = input[pos];
            err.line    = line;
            err.column  = col;
            err.message = "Caracter inesperado '" +
                          std::string(1, input[pos]) + "'";

            result.errors.push_back(err);
            result.success = false;

            // Avanzamos 1 caracter para continuar el análisis
            if (input[pos] == '\n') { line++; col = 1; }
            else                    { col++; }
            pos++;

        } else {
            // ── Token encontrado ──────────────────────────────
            // ¿Lo incluimos en el resultado?
            // Si skipWhitespace está activo, omitimos WHITESPACE
            if (!skipWhitespace ||
                match->tokenName != "WHITESPACE") {

                result.tokens.emplace_back(
                    match->tokenName,
                    match->lexeme,
                    match->line,
                    match->column
                );
            }
        }
    }

    // Agregamos el token EOF al final
    // El parser sintáctico lo necesita para saber que terminó el input
    result.tokens.emplace_back("EOF", "", line, col);

    // Adjuntamos el JSON del DFA para el frontend
    result.dfaJSON = dfa_.toJSON();

    return result;
}

// ════════════════════════════════════════════════════════════
//  toJSON() — serializa resultado completo para el API
// ════════════════════════════════════════════════════════════
std::string Tokenizer::toJSON() const {
    std::ostringstream json;
    json << "{\n";
    json << "  \"dfa\": " << dfa_.toJSON() << ",\n";
    json << "  \"rules\": [\n";

    for (size_t i = 0; i < spec_.rules.size(); i++) {
        const auto& r = spec_.rules[i];
        json << "    {\"priority\":" << r.priority
             << ", \"token\":\"" << r.tokenName
             << "\", \"pattern\":\"" << r.pattern << "\"}";
        if (i + 1 < spec_.rules.size()) json << ",";
        json << "\n";
    }

    json << "  ]\n}";
    return json.str();
}

} // namespace yalex