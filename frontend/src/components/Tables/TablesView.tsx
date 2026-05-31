// ============================================================
//  TablesView.tsx — Tablas SLR/LL1/LALR y lista de tokens
// ============================================================
import { useState } from 'react';
import { useStore } from '../../store/useStore';

type TableTab = 'tokens' | 'slr' | 'll1' | 'lalr' | 'grammar';

export function TablesView() {
  const { yalexResult, yaparResult } = useStore();
  const [tab, setTab] = useState<TableTab>('tokens');

  const tabs: { id: TableTab; label: string; available: boolean }[] = [
    { id: 'tokens',  label: 'Tokens',   available: !!yalexResult },
    { id: 'grammar', label: 'Gramática', available: !!yaparResult },
    { id: 'slr',     label: 'SLR(1)',   available: !!yaparResult },
    { id: 'll1',     label: 'LL(1)',    available: !!yaparResult },
    { id: 'lalr',    label: 'LALR(1)',  available: !!yaparResult },
  ];

  return (
    <div className="flex flex-col h-full">
      {/* Tabs */}
      <div className="flex gap-1 px-4 pt-3 border-b border-border">
        {tabs.map(t => (
          <button
            key={t.id}
            onClick={() => t.available && setTab(t.id)}
            className={`px-3 py-2 text-xs font-medium rounded-t-lg transition-all
              ${tab === t.id
                ? 'bg-bg-card text-text-primary border-b-2 border-accent-purple'
                : t.available
                  ? 'text-text-secondary hover:text-text-primary'
                  : 'text-text-muted cursor-not-allowed opacity-40'
              }`}
          >
            {t.label}
          </button>
        ))}
      </div>

      <div className="flex-1 overflow-auto p-4">
        {/* Tokens */}
        {tab === 'tokens' && yalexResult && (
          <div className="space-y-3">
            {yalexResult.errors.length > 0 && (
              <div className="bg-red-950/30 border border-red-800/50 rounded-lg p-3">
                <p className="text-red-400 text-xs font-medium mb-1">Errores léxicos:</p>
                {yalexResult.errors.map((e, i) => (
                  <p key={i} className="text-red-300 text-xs font-mono">
                    Línea {e.line}:{e.col} — {e.message}
                  </p>
                ))}
              </div>
            )}
            <table className="w-full text-xs">
              <thead>
                <tr className="border-b border-border">
                  <th className="text-left py-2 px-3 text-text-secondary">#</th>
                  <th className="text-left py-2 px-3 text-text-secondary">Tipo</th>
                  <th className="text-left py-2 px-3 text-text-secondary">Lexema</th>
                  <th className="text-left py-2 px-3 text-text-secondary">Línea</th>
                  <th className="text-left py-2 px-3 text-text-secondary">Col</th>
                </tr>
              </thead>
              <tbody>
                {yalexResult.tokens.map((t, i) => (
                  <tr key={i} className="border-b border-border/50 hover:bg-bg-secondary">
                    <td className="py-1.5 px-3 text-text-muted font-mono">{i}</td>
                    <td className="py-1.5 px-3">
                      <span className="px-2 py-0.5 rounded text-xs font-mono bg-accent-purple/20 text-purple-300">
                        {t.type}
                      </span>
                    </td>
                    <td className="py-1.5 px-3 font-mono text-accent-teal">{t.lexeme || '—'}</td>
                    <td className="py-1.5 px-3 text-text-secondary">{t.line}</td>
                    <td className="py-1.5 px-3 text-text-secondary">{t.col}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}

        {/* Gramática con FIRST/FOLLOW implícitos */}
        {tab === 'grammar' && yaparResult && (
          <div className="space-y-2">
            <p className="text-xs text-text-secondary mb-3">
              Símbolo inicial: <span className="font-mono text-accent-teal">{yaparResult.grammar.start}</span>
            </p>
            {yaparResult.grammar.productions.map(p => (
              <div key={p.id} className="flex items-center gap-3 py-1.5 px-3 rounded-lg bg-bg-secondary hover:bg-bg-card transition-colors">
                <span className="text-xs text-text-muted font-mono w-6">{p.id}.</span>
                <span className="text-xs font-mono text-accent-teal font-medium">{p.lhs}</span>
                <span className="text-xs text-text-muted">→</span>
                <span className="text-xs font-mono text-text-primary">
                  {p.rhs.length === 0 ? 'ε' : p.rhs.join(' ')}
                </span>
              </div>
            ))}
          </div>
        )}

        {/* Tabla SLR(1) */}
        {tab === 'slr' && yaparResult && (
          <div className="space-y-4">
            {yaparResult.slr.conflicts?.length > 0 && (
              <div className="bg-yellow-950/30 border border-yellow-700/50 rounded-lg p-3">
                <p className="text-yellow-400 text-xs font-medium mb-1">
                  ⚠️ {yaparResult.slr.conflicts.length} conflicto(s) detectado(s)
                </p>
              </div>
            )}
            <div className="overflow-x-auto">
              <table className="text-xs min-w-full">
                <thead>
                  <tr className="border-b border-border">
                    <th className="text-left py-2 px-3 text-text-secondary">Estado</th>
                    <th className="text-left py-2 px-3 text-text-secondary">Símbolo</th>
                    <th className="text-left py-2 px-3 text-text-secondary">Acción</th>
                  </tr>
                </thead>
                <tbody>
                  {yaparResult.slr.action?.map((entry, i) => (
                    <tr key={i} className="border-b border-border/50 hover:bg-bg-secondary">
                      <td className="py-1.5 px-3 font-mono text-text-secondary">{entry.state}</td>
                      <td className="py-1.5 px-3 font-mono text-accent-teal">{entry.symbol}</td>
                      <td className="py-1.5 px-3">
                        <span className={`px-2 py-0.5 rounded font-mono text-xs
                          ${entry.action.startsWith('s') ? 'bg-blue-900/40 text-blue-300' :
                            entry.action.startsWith('r') ? 'bg-orange-900/40 text-orange-300' :
                            entry.action === 'acc'       ? 'bg-green-900/40 text-green-300' :
                            'bg-red-900/40 text-red-300'}`}>
                          {entry.action}
                        </span>
                      </td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
          </div>
        )}

        {/* LL(1) */}
        {tab === 'll1' && yaparResult && (
          <div>
            <p className="text-xs text-text-secondary mb-3">
              {yaparResult.ll1?.table?.length > 0
                ? '✅ Tabla LL(1) construida'
                : '⚠️ Gramática no es LL(1)'}
            </p>
            <table className="text-xs w-full">
              <thead>
                <tr className="border-b border-border">
                  <th className="text-left py-2 px-3 text-text-secondary">NT</th>
                  <th className="text-left py-2 px-3 text-text-secondary">Terminal</th>
                  <th className="text-left py-2 px-3 text-text-secondary">Producción</th>
                </tr>
              </thead>
              <tbody>
                {yaparResult.ll1?.table?.map((entry: any, i: number) => (
                  <tr key={i} className="border-b border-border/50 hover:bg-bg-secondary">
                    <td className="py-1.5 px-3 font-mono text-accent-teal">{entry.nt}</td>
                    <td className="py-1.5 px-3 font-mono text-text-secondary">{entry.t}</td>
                    <td className="py-1.5 px-3 font-mono text-text-primary">p{entry.prod}</td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}

        {/* LALR */}
        {tab === 'lalr' && yaparResult && (
          <div>
            <p className="text-xs text-text-secondary mb-3">
              Tabla LALR(1) — {yaparResult.lalr ? '✅ Construida' : 'No disponible'}
            </p>
            <pre className="text-xs font-mono text-text-secondary bg-bg-secondary p-3 rounded-lg overflow-auto max-h-96">
              {JSON.stringify(yaparResult.lalr, null, 2)}
            </pre>
          </div>
        )}

        {/* Estado vacío */}
        {!yalexResult && !yaparResult && (
          <div className="flex items-center justify-center h-full text-text-secondary">
            <div className="text-center">
              <div className="text-4xl mb-3">📊</div>
              <p>Ejecuta el analizador para ver las tablas</p>
            </div>
          </div>
        )}
      </div>
    </div>
  );
}
