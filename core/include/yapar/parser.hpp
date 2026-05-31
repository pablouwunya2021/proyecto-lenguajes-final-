// ============================================================
//  parser.hpp — Motor de parsing LR
//
//  El parser LR usa un algoritmo de pila:
//
//  Pila:   [ estado0, símbolo1, estado1, símbolo2, estado2, ... ]
//  Input:  [ token1, token2, token3, ..., $ ]
//
//  Bucle principal:
//    s = estado en tope de pila
//    a = token actual del input
//    acción = ACTION[s][a]
//
//    Si acción = shift(t):
//      empuja (a, t) en la pila
//      avanza al siguiente token
//
//    Si acción = reduce(A → β):
//      saca |β| símbolos de la pila
//      t = estado en nuevo tope
//      empuja (A, GOTO[t][A])
//
//    Si acción = accept: ¡éxito!
//    Si acción = error: reportar error
//
//  El árbol sintáctico se construye durante las reducciones.
// ============================================================
#pragma once
#include "yapar/slr1_table.hpp"
#include "yapar/lalr_table.hpp"
#include "yalex/tokenizer.hpp"
#include <vector>
#include <string>
#include <memory>

namespace yapar {

// ── Nodo del árbol sintáctico ────────────────────────────────
struct ParseNode {
    std::string                        symbol;    // nombre del símbolo
    std::string                        lexeme;    // valor (solo hojas)
    int                                line = 0;
    int                                col  = 0;
    std::vector<std::shared_ptr<ParseNode>> children;

    bool isLeaf() const { return children.empty(); }

    // Serialización para D3.js (árbol jerárquico)
    std::string toJSON() const;
};

// ── Paso del parsing (para visualización) ────────────────────
// Guardamos cada paso para que el frontend lo pueda animar
struct ParseStep {
    std::vector<int>         stateStack;   // pila de estados
    std::vector<std::string> symbolStack;  // pila de símbolos
    std::string              currentToken;
    std::string              action;       // "shift 3", "reduce E→E+T", etc.
};

// ── Resultado del parsing ─────────────────────────────────────
struct ParseResult {
    bool                         success;
    std::shared_ptr<ParseNode>   tree;     // árbol sintáctico (si success)
    std::vector<ParseStep>       steps;    // traza paso a paso
    std::vector<std::string>     errors;   // mensajes de error con línea:col
};

// ── Parser LR ────────────────────────────────────────────────
class Parser {
public:
    // Configura el parser con una tabla SLR(1) o LALR
    void useTable(const SLR1Table& table, const Grammar& grammar);
    void useTable(const LALRTable& table, const Grammar& grammar);

    // parse() — toma la lista de tokens de YALex y produce el árbol
    ParseResult parse(const std::vector<yalex::Token>& tokens) const;

private:
    // Punteros a las tablas (uno de los dos será nulo)
    const SLR1Table*  slrTable_  = nullptr;
    const LALRTable*  lalrTable_ = nullptr;
    const Grammar*    grammar_   = nullptr;

    // Interfaz unificada de consulta (delega a la tabla activa)
    Action getAction(int state, const std::string& terminal) const;
    int    getGoto(int state, const std::string& nonTerminal) const;
};

} // namespace yapar
