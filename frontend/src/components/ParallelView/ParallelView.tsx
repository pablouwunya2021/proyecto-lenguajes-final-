// ============================================================
//  ParallelView.tsx — Visualización del análisis paralelo
//  de conflictos shift/reduce
// ============================================================
import { useStore } from '../../store/useStore';

export function ParallelView() {
  const { yaparResult } = useStore();

  if (!yaparResult) {
    return (
      <div className="flex items-center justify-center h-full text-text-secondary">
        <div className="text-center">
          <div className="text-5xl mb-4">⚡</div>
          <p className="text-lg font-medium">Análisis paralelo de conflictos</p>
          <p className="text-sm mt-2 text-text-muted">Ejecuta YAPar para ver el análisis</p>
        </div>
      </div>
    );
  }

  const conflicts = yaparResult.conflicts || [];

  if (conflicts.length === 0) {
    return (
      <div className="flex items-center justify-center h-full text-text-secondary">
        <div className="text-center">
          <div className="text-5xl mb-4">✅</div>
          <p className="text-lg font-medium text-accent-green">Sin conflictos</p>
          <p className="text-sm mt-2">La gramática es SLR(1) limpia — no se necesita análisis paralelo</p>
        </div>
      </div>
    );
  }

  return (
    <div className="p-4 space-y-4 overflow-auto h-full">
      <div className="flex items-center gap-3 mb-4">
        <div className="w-2 h-2 rounded-full bg-accent-orange animate-pulse" />
        <h3 className="text-sm font-semibold text-text-primary">
          {conflicts.length} conflicto(s) analizados en paralelo
        </h3>
      </div>

      {/* Leyenda de threads */}
      <div className="card mb-4">
        <p className="text-xs text-text-secondary mb-2">
          Cada conflicto fue analizado por un <code className="font-mono text-accent-teal">std::thread</code> independiente:
        </p>
        <div className="flex flex-wrap gap-2">
          {conflicts.map((_, i) => (
            <div key={i} className="flex items-center gap-1.5 px-2 py-1 bg-bg-secondary rounded-lg">
              <div className="w-2 h-2 rounded-full bg-accent-purple" />
              <span className="text-xs font-mono text-text-secondary">Thread {i}</span>
            </div>
          ))}
        </div>
      </div>

      {/* Conflictos */}
      {conflicts.map((c, i) => (
        <div key={i} className="card border-l-4 border-l-accent-orange animate-slide-up"
             style={{ animationDelay: `${i * 100}ms` }}>
          <div className="flex items-start justify-between mb-2">
            <div className="flex items-center gap-2">
              <span className="text-xs font-mono text-accent-orange bg-orange-900/30 px-2 py-0.5 rounded">
                Thread {i}
              </span>
              <span className="text-xs text-text-secondary">Shift/Reduce</span>
            </div>
            <span className="text-xs px-2 py-0.5 rounded bg-green-900/30 text-accent-green">
              Resuelto ✓
            </span>
          </div>

          <p className="text-xs font-mono text-text-secondary mb-3 bg-bg-secondary p-2 rounded">
            {c.description}
          </p>

          <div className="grid grid-cols-2 gap-3 mb-3">
            <div className="bg-blue-900/20 border border-blue-800/30 rounded-lg p-2">
              <p className="text-xs text-blue-400 font-medium mb-1">Opción A: Shift</p>
              <p className="text-xs text-text-secondary">
                Consume {c.shiftDepth} token(s) más → parse más específico
              </p>
            </div>
            <div className="bg-orange-900/20 border border-orange-800/30 rounded-lg p-2">
              <p className="text-xs text-orange-400 font-medium mb-1">Opción B: Reduce</p>
              <p className="text-xs text-text-secondary">
                Reduce {c.reduceLength} símbolo(s) → cierra producción
              </p>
            </div>
          </div>

          <div className="bg-accent-purple/10 border border-accent-purple/30 rounded-lg p-2">
            <p className="text-xs text-purple-300 font-medium">→ Decisión del Thread {i}:</p>
            <p className="text-xs text-text-secondary mt-0.5">{c.reason}</p>
          </div>
        </div>
      ))}
    </div>
  );
}
