#include "yapar/parser.hpp"
#include <iostream>
#include <sstream>
#include <stdexcept>

namespace yapar {

void Parser::useTable(const SLR1Table& table, const Grammar& grammar) {
    slrTable_  = &table;
    lalrTable_ = nullptr;
    grammar_   = &grammar;
}

void Parser::useTable(const LALRTable& table, const Grammar& grammar) {
    lalrTable_ = &table;
    slrTable_  = nullptr;
    grammar_   = &grammar;
}

Action Parser::getAction(int state, const std::string& terminal) const {
    if (slrTable_)  return slrTable_->getAction(state, terminal);
    if (lalrTable_) return lalrTable_->getAction(state, terminal);
    return Action::error();
}

int Parser::getGoto(int state, const std::string& nonTerminal) const {
    if (slrTable_)  return slrTable_->getGoto(state, nonTerminal);
    if (lalrTable_) return lalrTable_->getGoto(state, nonTerminal);
    return -1;
}

// ════════════════════════════════════════════════════════════
//  parse() — algoritmo LR con pila
// ════════════════════════════════════════════════════════════
ParseResult Parser::parse(const std::vector<yalex::Token>& tokens) const {
    ParseResult result;
    result.success = false;

    if (!grammar_) {
        result.errors.push_back("Parser no inicializado");
        return result;
    }

    // Pila de estados y pila de nodos del árbol sintáctico
    std::vector<int>                         stateStack  = {0};
    std::vector<std::shared_ptr<ParseNode>>  nodeStack;

    // Índice del token actual
    size_t tokenIdx = 0;

    auto currentToken = [&]() -> const yalex::Token& {
        return tokens[std::min(tokenIdx, tokens.size()-1)];
    };

    int maxSteps = 10000; // límite de seguridad para evitar loops infinitos
    int steps    = 0;

    while (steps++ < maxSteps) {
        int         s = stateStack.back();
        // Traducimos "EOF" → "$" (el marcador de fin que usa la tabla)
        std::string a = currentToken().type;
        if (a == "EOF") a = "$";

        Action action = getAction(s, a);

        // ── Guardar paso para visualización ──────────────────
        ParseStep step;
        step.stateStack  = stateStack;
        step.currentToken = a + "(\"" + currentToken().lexeme + "\")";

        switch (action.type) {

            case ActionType::SHIFT: {
                // Avanzamos: empujamos el estado y creamos nodo hoja
                step.action = "shift " + std::to_string(action.value);
                result.steps.push_back(step);

                stateStack.push_back(action.value);

                auto leaf = std::make_shared<ParseNode>();
                leaf->symbol = currentToken().type;
                leaf->lexeme = currentToken().lexeme;
                leaf->line   = currentToken().line;
                leaf->col    = currentToken().column;
                nodeStack.push_back(leaf);

                tokenIdx++;
                break;
            }

            case ActionType::REDUCE: {
                // Reducimos: sacamos |rhs| elementos y empujamos el LHS
                int prodId = action.value;
                if (prodId >= (int)grammar_->getProductions().size()) {
                    result.errors.push_back("Linea " +
                        std::to_string(currentToken().line) +
                        ": ID de produccion invalido: " + std::to_string(prodId));
                    return result;
                }

                const Production& prod = grammar_->getProductions()[prodId];

                step.action = "reduce " + prod.lhs.name + " → ";
                for (const auto& sym : prod.rhs) step.action += sym.name + " ";
                result.steps.push_back(step);

                // Creamos nodo interno del árbol
                auto node = std::make_shared<ParseNode>();
                node->symbol = prod.lhs.name;

                int rhsSize = (int)prod.rhs.size();
                // Sacamos rhsSize elementos de las pilas
                for (int i = 0; i < rhsSize; i++) {
                    if (stateStack.size() > 1) stateStack.pop_back();
                }
                // Los hijos se agregan en orden correcto
                if ((int)nodeStack.size() >= rhsSize) {
                    for (int i = rhsSize - 1; i >= 0; i--) {
                        node->children.insert(node->children.begin(),
                                              nodeStack[nodeStack.size() - rhsSize + i]);
                    }
                    for (int i = 0; i < rhsSize; i++) nodeStack.pop_back();
                }

                // GOTO[tope actual][LHS]
                int topState = stateStack.back();
                int gotoState = getGoto(topState, prod.lhs.name);
                if (gotoState == -1) {
                    result.errors.push_back("Linea " +
                        std::to_string(currentToken().line) +
                        " (regla yapar:" + std::to_string(prod.sourceLine) +
                        "): Error GOTO: estado " + std::to_string(topState) +
                        " con " + prod.lhs.name);
                    return result;
                }

                stateStack.push_back(gotoState);
                nodeStack.push_back(node);
                break;
            }

            case ActionType::ACCEPT: {
                step.action = "accept";
                result.steps.push_back(step);
                result.success = true;
                if (!nodeStack.empty())
                    result.tree = nodeStack.back();
                return result;
            }

            case ActionType::ERROR:
            default: {
                std::ostringstream err;
                err << "Error sintactico en linea " << currentToken().line
                    << ":" << currentToken().column
                    << " — token inesperado '" << currentToken().lexeme
                    << "' (" << a << ")";
                result.errors.push_back(err.str());
                return result;
            }
        }
    }

    result.errors.push_back("Parsing excedio el limite de pasos (posible loop)");
    return result;
}

// ── ParseNode::toJSON ─────────────────────────────────────────
std::string ParseNode::toJSON() const {
    std::ostringstream j;
    j << "{\"name\":\"" << symbol << "\"";
    if (!lexeme.empty()) j << ",\"lexeme\":\"" << lexeme << "\"";
    if (!children.empty()) {
        j << ",\"children\":[";
        for (size_t i = 0; i < children.size(); i++) {
            if (i) j << ",";
            j << children[i]->toJSON();
        }
        j << "]";
    }
    j << "}";
    return j.str();
}

} // namespace yapar
