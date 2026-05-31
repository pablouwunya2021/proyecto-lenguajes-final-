// ============================================================
//  conflict_resolver.hpp — Resolución paralela de conflictos
//
//  Un conflicto shift/reduce ocurre cuando en un estado,
//  para un mismo terminal, la tabla tiene DOS acciones posibles:
//    - shift (seguir leyendo)
//    - reduce (aplicar una producción)
//
//  Ejemplo clásico — el "dangling else":
//    if E then S
//    if E then S else S
//  Cuando el parser ve "else" después de S, puede:
//    - shift (el else pertenece al if más cercano) ← convención
//    - reduce (cerrar el if sin else)
//
//  ¿Cómo lo resolvemos con PARALELISMO?
//  Lanzamos std::thread por cada conflicto para analizar
//  simultáneamente los "caminos alternativos" que tomaría
//  el parser con cada resolución, y reportamos cuál es
//  más prometedor según heurísticas.
// ============================================================
#pragma once
#include "yapar/slr1_table.hpp"
#include "yapar/lalr_table.hpp"
#include <vector>
#include <string>
#include <functional>
#include <thread>
#include <mutex>
#include <atomic>

namespace yapar {

// ── Estrategia de resolución ─────────────────────────────────
enum class ResolutionStrategy {
    PREFER_SHIFT,   // shift gana (convención estándar, ej: dangling else)
    PREFER_REDUCE,  // reduce gana
    REPORT_ERROR,   // marcar como error (gramática ambigua)
    FIRST_RULE      // la producción más temprana tiene prioridad
};

// ── Resultado del análisis de un camino alternativo ──────────
struct PathAnalysis {
    std::string        conflictDescription;
    ResolutionStrategy suggested;
    std::string        reason;       // explicación legible
    int                shiftDepth;   // cuántos tokens más consume shift
    int                reduceLength; // largo del lado derecho reducido
};

// ── Resolutor de conflictos ───────────────────────────────────
class ConflictResolver {
public:
    ConflictResolver(ResolutionStrategy defaultStrategy = ResolutionStrategy::PREFER_SHIFT)
        : defaultStrategy_(defaultStrategy) {}

    // resolveAll() — analiza todos los conflictos en paralelo
    // Cada conflicto se analiza en su propio std::thread
    std::vector<PathAnalysis> resolveAll(
        const std::vector<Conflict>& conflicts,
        const Grammar&               grammar
    );

    // Callback para notificar progreso al frontend en tiempo real
    // Se llama desde los threads con mutex protegido
    using ProgressCallback = std::function<void(int done, int total, const PathAnalysis&)>;
    void setProgressCallback(ProgressCallback cb) { progressCb_ = cb; }

    // Aplica las resoluciones a una tabla SLR(1)
    void applyResolutions(SLR1Table& table,
                          const std::vector<Conflict>& conflicts,
                          const std::vector<PathAnalysis>& analyses);

    std::string toJSON(const std::vector<PathAnalysis>& analyses) const;

private:
    ResolutionStrategy defaultStrategy_;
    ProgressCallback   progressCb_;
    std::mutex         mutex_;            // protege progressCb_ y resultados
    std::atomic<int>   doneTasks_{0};

    // Analiza un conflicto individual (corre en su propio thread)
    PathAnalysis analyzeConflict(const Conflict& conflict,
                                  const Grammar&  grammar);
};

} // namespace yapar
