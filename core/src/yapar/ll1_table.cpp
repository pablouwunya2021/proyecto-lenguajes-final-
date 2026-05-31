#include "yapar/ll1_table.hpp"
#include <iostream>
#include <sstream>

namespace yapar {

void LL1Table::setEntry(const std::string& nt, const std::string& t, int prodId) {
    auto& cell = table_[nt][t];
    if (cell != 0 && cell != prodId) {
        // Conflicto LL(1)
        bool found = false;
        for (auto& c : conflicts_)
            if (c.nonTerminal == nt && c.terminal == t) {
                c.productions.push_back(prodId);
                found = true; break;
            }
        if (!found) conflicts_.push_back({nt, t, {cell, prodId}});
    } else {
        cell = prodId;
    }
}

void LL1Table::build(const Grammar& grammar) {
    grammar_ = &grammar;
    table_.clear();
    conflicts_.clear();

    // Inicializar todas las celdas en -1
    for (const auto& nt : grammar.getNonTerminals())
        for (const auto& t : grammar.getTerminals())
            table_[nt.name][t.name] = -1;

    for (const auto& prod : grammar.getProductions()) {
        std::string A = prod.lhs.name;

        // FIRST(α)
        auto firstAlpha = prod.isEpsilon()
            ? std::set<std::string>{EPSILON}
            : grammar.firstOfSequence(prod.rhs);

        // Para cada terminal a ∈ FIRST(α) - {ε}: M[A][a] = prod
        for (const auto& a : firstAlpha) {
            if (a != EPSILON) setEntry(A, a, prod.id);
        }

        // Si ε ∈ FIRST(α): para cada b ∈ FOLLOW(A): M[A][b] = prod
        if (firstAlpha.count(EPSILON)) {
            for (const auto& b : grammar.getFollow(A))
                setEntry(A, b, prod.id);
        }
    }
}

int LL1Table::getEntry(const std::string& nt, const std::string& t) const {
    auto ri = table_.find(nt);
    if (ri == table_.end()) return -1;
    auto ci = ri->second.find(t);
    if (ci == ri->second.end()) return -1;
    return ci->second;
}

std::string LL1Table::toJSON() const {
    std::ostringstream j;
    j << "{\n  \"table\": [\n";
    bool first = true;
    for (const auto& [nt, row] : table_) {
        for (const auto& [t, prod] : row) {
            if (prod == -1) continue;
            if (!first) j << ",\n"; first = false;
            j << "    {\"nt\":\"" << nt << "\",\"t\":\"" << t
              << "\",\"prod\":" << prod << "}";
        }
    }
    j << "\n  ]\n}";
    return j.str();
}

void LL1Table::print() const {
    std::cout << "=== Tabla LL(1) ===\n";
    if (!conflicts_.empty()) {
        std::cout << "⚠️  Gramática NO es LL(1): " << conflicts_.size() << " conflicto(s)\n";
    } else {
        std::cout << "✅ Gramática es LL(1)\n";
    }
    for (const auto& [nt, row] : table_) {
        std::cout << nt << ": ";
        for (const auto& [t, prod] : row)
            if (prod != -1) std::cout << t << "→p" << prod << " ";
        std::cout << "\n";
    }
}

} // namespace yapar
