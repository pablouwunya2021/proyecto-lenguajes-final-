// ============================================================
//  conflict_resolver.cpp — Análisis paralelo de conflictos
// ============================================================
#include "yapar/conflict_resolver.hpp"
#include <sstream>
#include <iostream>

namespace yapar {

// ════════════════════════════════════════════════════════════
//  resolveAll() — lanza un thread por cada conflicto
//
//  Cada thread analiza independientemente su conflicto.
//  Usamos mutex para proteger el vector de resultados y
//  el callback de progreso.
// ════════════════════════════════════════════════════════════
std::vector<PathAnalysis> ConflictResolver::resolveAll(
    const std::vector<Conflict>& conflicts,
    const Grammar&               grammar
) {
    if (conflicts.empty()) return {};

    std::vector<PathAnalysis> results(conflicts.size());
    std::vector<std::thread>  threads;
    doneTasks_ = 0;
    int total = (int)conflicts.size();

    // Lanzamos un thread por cada conflicto
    for (int i = 0; i < total; i++) {
        threads.emplace_back([this, i, &conflicts, &grammar, &results, total]() {
            // Cada thread analiza su conflicto de forma independiente
            PathAnalysis analysis = analyzeConflict(conflicts[i], grammar);

            // Guardamos el resultado con mutex (escritura concurrente segura)
            {
                std::lock_guard<std::mutex> lock(mutex_);
                results[i] = analysis;
                doneTasks_++;

                // Notificamos el progreso al frontend
                if (progressCb_) {
                    progressCb_(doneTasks_.load(), total, analysis);
                }
            }

            std::cout << "[Thread " << i << "] Conflicto analizado: "
                      << analysis.conflictDescription << "\n"
                      << "  → Sugerencia: " << analysis.reason << "\n";
        });
    }

    // Esperamos a que todos los threads terminen
    for (auto& t : threads) t.join();

    return results;
}

// ════════════════════════════════════════════════════════════
//  analyzeConflict() — analiza un conflicto individual
//
//  Heurísticas aplicadas:
//  1. Si es shift/reduce en el mismo estado con MISMO símbolo:
//     - Verificamos la longitud del RHS de la reducción
//     - Shift más profundo generalmente genera árboles más correctos
//  2. Regla de precedencia: producción con menor id tiene prioridad
//  3. El "dangling else" siempre se resuelve con shift
// ════════════════════════════════════════════════════════════
PathAnalysis ConflictResolver::analyzeConflict(
    const Conflict& conflict,
    const Grammar&  grammar
) {
    PathAnalysis result;
    result.conflictDescription = conflict.description;

    const bool isShiftReduce =
        (conflict.existing.type == ActionType::SHIFT  && conflict.incoming.type == ActionType::REDUCE) ||
        (conflict.existing.type == ActionType::REDUCE && conflict.incoming.type == ActionType::SHIFT);

    const bool isReduceReduce =
        conflict.existing.type == ActionType::REDUCE &&
        conflict.incoming.type == ActionType::REDUCE;

    if (isShiftReduce) {
        // ── Conflicto Shift/Reduce ────────────────────────────
        // Determinamos cuál es el shift y cuál el reduce
        int reduceId = (conflict.existing.type == ActionType::REDUCE)
                       ? conflict.existing.value
                       : conflict.incoming.value;

        const Production& reduceProd = grammar.getProductions()[reduceId];
        result.reduceLength = (int)reduceProd.rhs.size();
        result.shiftDepth   = 1; // el shift siempre consume 1 token

        // Heurística: preferir shift (convención estándar para dangling else)
        // Razón: el shift produce un parse más específico (más tokens contexto)
        result.suggested = ResolutionStrategy::PREFER_SHIFT;
        result.reason    = "Shift/Reduce: preferimos shift (convención estandar). "
                           "Reduccion seria: " + reduceProd.lhs.name +
                           " → " + std::to_string(result.reduceLength) + " simbolos";

    } else if (isReduceReduce) {
        // ── Conflicto Reduce/Reduce ────────────────────────────
        // Dos producciones compiten. Elegimos la de menor id (apareció primero)
        int p1 = conflict.existing.value;
        int p2 = conflict.incoming.value;
        result.reduceLength = 0;
        result.shiftDepth   = 0;
        result.suggested    = ResolutionStrategy::FIRST_RULE;
        result.reason       = "Reduce/Reduce: se prefiere produccion " +
                              std::to_string(std::min(p1, p2)) +
                              " (regla mas temprana en la gramatica)";
    } else {
        result.suggested = defaultStrategy_;
        result.reason    = "Conflicto no clasificado. Usando estrategia por defecto.";
    }

    return result;
}

// ════════════════════════════════════════════════════════════
//  applyResolutions() — aplica las decisiones a la tabla
// ════════════════════════════════════════════════════════════
void ConflictResolver::applyResolutions(
    SLR1Table&                       table,
    const std::vector<Conflict>&     conflicts,
    const std::vector<PathAnalysis>& analyses
) {
    // La tabla ya tomó decisiones durante build() (prefiere shift).
    // Aquí podríamos forzar una estrategia diferente si se desea.
    // Por ahora logueamos las decisiones.
    for (size_t i = 0; i < conflicts.size() && i < analyses.size(); i++) {
        std::cout << "[Resolver] " << analyses[i].reason << "\n";
    }
}

std::string ConflictResolver::toJSON(const std::vector<PathAnalysis>& analyses) const {
    std::ostringstream j;
    j << "[\n";
    for (size_t i = 0; i < analyses.size(); i++) {
        const auto& a = analyses[i];
        j << "  {\"description\":\"" << a.conflictDescription
          << "\",\"reason\":\"" << a.reason
          << "\",\"shiftDepth\":" << a.shiftDepth
          << ",\"reduceLength\":" << a.reduceLength << "}";
        if (i+1 < analyses.size()) j << ",";
        j << "\n";
    }
    j << "]";
    return j.str();
}

} // namespace yapar
