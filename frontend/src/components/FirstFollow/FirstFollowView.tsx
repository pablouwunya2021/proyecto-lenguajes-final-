// ============================================================
//  FirstFollowView.tsx — Tablas FIRST y FOLLOW
// ============================================================
import { useState, useMemo } from 'react';
import { useStore } from '../../store/useStore';
import type { ModuleTheme } from '../../types/themes';

interface Props { theme: ModuleTheme }

type Tab = 'first' | 'follow' | 'combined';

export function FirstFollowView({ theme }: Props) {
  const { yaparResult } = useStore();
  const [tab, setTab] = useState<Tab>('combined');

  const grammar    = yaparResult?.grammar;
  const firstMap  = (grammar as any)?.first  as Record<string, string[]> | undefined;
  const followMap = (grammar as any)?.follow as Record<string, string[]> | undefined;
  const prods     = grammar?.productions ?? [];

  // No-terminales en orden de aparición en producciones
  const nonTerminals = useMemo(() => {
    if (!prods.length) return [];
    const seen = new Set<string>();
    const order: string[] = [];
    for (const p of prods) {
      if (!seen.has(p.lhs)) { seen.add(p.lhs); order.push(p.lhs); }
    }
    return order;
  }, [prods]);

  // Todos los terminales que aparecen en FIRST o FOLLOW (para encabezados de columna)
  const allTerminals = useMemo(() => {
    const set = new Set<string>();
    for (const nt of nonTerminals) {
      for (const t of firstMap?.[nt] ?? [])  set.add(t);
      for (const t of followMap?.[nt] ?? []) set.add(t);
    }
    // Ordenar: ε primero, $ al final, resto alfabético
    return Array.from(set).sort((a, b) => {
      if (a === 'ε') return -1; if (b === 'ε') return 1;
      if (a === '$') return 1;  if (b === '$') return -1;
      return a.localeCompare(b);
    });
  }, [nonTerminals, firstMap, followMap]);

  if (!yaparResult) {
    return (
      <div className="flex items-center justify-center h-full" style={{ color: theme.textMuted }}>
        <div className="text-center">
          <div style={{ fontSize: '48px', marginBottom: '16px' }}>∅</div>
          <p style={{ fontSize: '16px', fontWeight: 600, color: theme.textPrimary }}>
            Tablas FIRST y FOLLOW
          </p>
          <p style={{ fontSize: '13px', marginTop: '8px', fontFamily: theme.fontFamily }}>
            Ejecuta el análisis con un .yapar para ver los conjuntos
          </p>
        </div>
      </div>
    );
  }

  if (!firstMap || !followMap || nonTerminals.length === 0) {
    return (
      <div className="flex items-center justify-center h-full" style={{ color: theme.textMuted }}>
        <p style={{ fontFamily: theme.fontFamily }}>Sin datos de FIRST/FOLLOW disponibles</p>
      </div>
    );
  }

  // ── Helpers de color ────────────────────────────────────────
  const tokenBg = (tok: string, kind: 'first' | 'follow') => {
    if (tok === 'ε') return { bg: '#1a1a00', color: '#fbbf24', border: '#fbbf2440' };
    if (tok === '$') return { bg: '#001a1a', color: '#22d3ee', border: '#22d3ee40' };
    if (kind === 'first')  return { bg: '#001833', color: '#60a5fa', border: '#60a5fa40' };
    return { bg: '#002200', color: '#4ade80', border: '#4ade8040' };
  };

  // ── Sub-tabs ─────────────────────────────────────────────────
  const TABS: { id: Tab; label: string }[] = [
    { id: 'combined', label: 'Vista combinada' },
    { id: 'first',    label: 'FIRST'           },
    { id: 'follow',   label: 'FOLLOW'          },
  ];

  return (
    <div className="flex flex-col h-full" style={{ fontFamily: theme.fontFamily }}>

      {/* Sub-tabs */}
      <div className="flex items-center gap-1 px-3 py-2 border-b flex-shrink-0"
           style={{ background: theme.bgSecondary, borderColor: theme.borderColor }}>
        {TABS.map(t => {
          const active = tab === t.id;
          return (
            <button key={t.id} onClick={() => setTab(t.id)}
                    className="px-3 py-1.5 rounded-lg text-xs font-medium transition-all"
                    style={{
                      background: active ? theme.bgCard : 'transparent',
                      border:     `1px solid ${active ? theme.accentA : 'transparent'}`,
                      color:      active ? theme.accentA : theme.textMuted,
                      cursor: 'pointer',
                    }}>
              {t.label}
            </button>
          );
        })}

        {/* Leyenda rápida */}
        <div className="flex gap-3 ml-4">
          {[
            { label: 'FIRST',  bg: '#001833', color: '#60a5fa' },
            { label: 'FOLLOW', bg: '#002200', color: '#4ade80' },
            { label: 'ε',      bg: '#1a1a00', color: '#fbbf24' },
            { label: '$',      bg: '#001a1a', color: '#22d3ee' },
          ].map(l => (
            <span key={l.label} style={{
              fontSize: '10px', fontFamily: 'JetBrains Mono, monospace',
              padding: '2px 7px', borderRadius: '4px',
              background: l.bg, color: l.color,
            }}>
              {l.label}
            </span>
          ))}
        </div>
      </div>

      {/* Contenido */}
      <div className="flex-1 overflow-auto p-4">

        {/* ── Vista combinada ──────────────────────────────── */}
        {tab === 'combined' && (
          <div>
            <p style={{ fontSize: '12px', color: theme.textMuted, marginBottom: '16px' }}>
              Cada fila es un no-terminal. Las columnas azules son FIRST, las verdes son FOLLOW.
              Un token en ambos conjuntos aparece en la misma celda.
            </p>
            <div style={{ overflowX: 'auto' }}>
              <table style={{
                borderCollapse: 'collapse', width: 'max-content',
                fontSize: '12px', border: `1px solid ${theme.borderColor}`,
              }}>
                <thead>
                  <tr>
                    <th rowSpan={2} style={{
                      padding: '8px 16px', background: theme.bgCard,
                      color: theme.textPrimary, fontFamily: 'JetBrains Mono, monospace',
                      borderRight: `2px solid ${theme.accentA}44`,
                      borderBottom: `1px solid ${theme.borderColor}`,
                      textAlign: 'left', whiteSpace: 'nowrap',
                    }}>
                      No-terminal
                    </th>
                    {allTerminals.map(t => {
                      const inFirst  = nonTerminals.some(nt => firstMap[nt]?.includes(t));
                      const inFollow = nonTerminals.some(nt => followMap[nt]?.includes(t));
                      const color = t === 'ε' ? '#fbbf24' : t === '$' ? '#22d3ee'
                                  : (inFirst && inFollow) ? '#a78bfa'
                                  : inFirst ? '#60a5fa' : '#4ade80';
                      return (
                        <th key={t} style={{
                          padding: '5px 10px', textAlign: 'center',
                          fontFamily: 'JetBrains Mono, monospace', fontWeight: 700,
                          color, background: theme.bgSecondary,
                          borderRight: `1px solid ${theme.borderColor}`,
                          borderBottom: `2px solid ${color}44`,
                          whiteSpace: 'nowrap',
                        }}>
                          {t}
                        </th>
                      );
                    })}
                  </tr>
                </thead>
                <tbody>
                  {nonTerminals.map((nt, ni) => {
                    const myFirst  = new Set(firstMap[nt]  ?? []);
                    const myFollow = new Set(followMap[nt] ?? []);
                    return (
                      <tr key={nt} style={{
                        background: ni % 2 === 0 ? theme.bg : theme.bgSecondary,
                      }}>
                        {/* No-terminal */}
                        <td style={{
                          padding: '6px 16px', fontFamily: 'JetBrains Mono, monospace',
                          fontWeight: 700, color: '#a78bfa',
                          background: theme.bgCard, whiteSpace: 'nowrap',
                          borderRight: `2px solid ${theme.accentA}44`,
                          borderBottom: `1px solid ${theme.borderColor}`,
                        }}>
                          {nt}
                        </td>
                        {/* Celdas por terminal */}
                        {allTerminals.map(tok => {
                          const inF  = myFirst.has(tok);
                          const inFo = myFollow.has(tok);
                          if (!inF && !inFo) {
                            return (
                              <td key={tok} style={{
                                padding: '6px 10px', textAlign: 'center',
                                color: theme.textMuted, fontFamily: 'JetBrains Mono, monospace',
                                borderRight: `1px solid ${theme.borderColor}`,
                                borderBottom: `1px solid ${theme.borderColor}`,
                              }}>
                                —
                              </td>
                            );
                          }
                          return (
                            <td key={tok} style={{
                              padding: '4px 8px', textAlign: 'center',
                              borderRight: `1px solid ${theme.borderColor}`,
                              borderBottom: `1px solid ${theme.borderColor}`,
                            }}>
                              <div style={{ display: 'flex', flexDirection: 'column', gap: '2px', alignItems: 'center' }}>
                                {inF && (
                                  <span style={{
                                    fontSize: '10px', fontFamily: 'JetBrains Mono, monospace',
                                    padding: '1px 6px', borderRadius: '3px',
                                    ...(() => { const c = tokenBg(tok, 'first'); return { background: c.bg, color: c.color, border: `1px solid ${c.border}` }; })(),
                                  }}>
                                    F
                                  </span>
                                )}
                                {inFo && (
                                  <span style={{
                                    fontSize: '10px', fontFamily: 'JetBrains Mono, monospace',
                                    padding: '1px 6px', borderRadius: '3px',
                                    ...(() => { const c = tokenBg(tok, 'follow'); return { background: c.bg, color: c.color, border: `1px solid ${c.border}` }; })(),
                                  }}>
                                    FO
                                  </span>
                                )}
                              </div>
                            </td>
                          );
                        })}
                      </tr>
                    );
                  })}
                </tbody>
              </table>
            </div>

            {/* Leyenda de celdas */}
            <div className="flex gap-4 mt-4 flex-wrap">
              {[
                { label: 'F  = en FIRST(NT)',       bg: '#001833', color: '#60a5fa' },
                { label: 'FO = en FOLLOW(NT)',       bg: '#002200', color: '#4ade80' },
                { label: 'ε  = puede derivar vacío', bg: '#1a1a00', color: '#fbbf24' },
                { label: '$  = fin de cadena',       bg: '#001a1a', color: '#22d3ee' },
              ].map(l => (
                <span key={l.label} style={{
                  fontSize: '11px', color: l.color, fontFamily: theme.fontFamily,
                  display: 'flex', alignItems: 'center', gap: '6px',
                }}>
                  <span style={{
                    padding: '2px 7px', borderRadius: '4px',
                    background: l.bg, fontFamily: 'monospace', fontSize: '10px',
                  }}>
                    {l.label.split('=')[0].trim()}
                  </span>
                  {l.label.split('=')[1]}
                </span>
              ))}
            </div>
          </div>
        )}

        {/* ── Vista FIRST ─────────────────────────────────── */}
        {tab === 'first' && (
          <SetTable
            title="FIRST"
            subtitle="FIRST(A) = terminales que pueden aparecer al inicio de una derivación de A"
            sets={firstMap}
            nonTerminals={nonTerminals}
            kind="first"
            theme={theme}
          />
        )}

        {/* ── Vista FOLLOW ────────────────────────────────── */}
        {tab === 'follow' && (
          <SetTable
            title="FOLLOW"
            subtitle="FOLLOW(A) = terminales que pueden aparecer inmediatamente después de A en alguna derivación"
            sets={followMap}
            nonTerminals={nonTerminals}
            kind="follow"
            theme={theme}
          />
        )}
      </div>
    </div>
  );
}

// ── Tabla individual FIRST o FOLLOW ──────────────────────────
function SetTable({
  title, subtitle, sets, nonTerminals, kind, theme,
}: {
  title:         string;
  subtitle:      string;
  sets:          Record<string, string[]>;
  nonTerminals:  string[];
  kind:          'first' | 'follow';
  theme:         ModuleTheme;
}) {
  const accentColor = kind === 'first' ? '#60a5fa' : '#4ade80';
  const accentBg    = kind === 'first' ? '#001833' : '#002200';

  return (
    <div>
      {/* Header */}
      <div className="rounded-xl p-4 mb-5"
           style={{ background: `${accentBg}`, border: `1px solid ${accentColor}33` }}>
        <p style={{ fontSize: '16px', fontWeight: 700, color: accentColor,
                    fontFamily: 'JetBrains Mono, monospace', marginBottom: '4px' }}>
          {title}(A)
        </p>
        <p style={{ fontSize: '12px', color: theme.textMuted, fontFamily: theme.fontFamily }}>
          {subtitle}
        </p>
      </div>

      {/* Tabla */}
      <div style={{ overflowX: 'auto' }}>
        <table style={{
          borderCollapse: 'collapse', width: '100%',
          fontSize: '13px', border: `1px solid ${theme.borderColor}`,
        }}>
          <thead>
            <tr style={{ background: theme.bgCard }}>
              <th style={{
                padding: '8px 16px', textAlign: 'left',
                fontFamily: 'JetBrains Mono, monospace', fontWeight: 700,
                color: '#a78bfa',
                borderRight: `2px solid ${accentColor}44`,
                borderBottom: `2px solid ${accentColor}44`,
                whiteSpace: 'nowrap',
              }}>
                No-terminal
              </th>
              <th style={{
                padding: '8px 16px', textAlign: 'left',
                fontFamily: theme.fontFamily, fontWeight: 600,
                color: theme.textSecondary,
                borderBottom: `2px solid ${accentColor}44`,
              }}>
                {title}( · )
              </th>
            </tr>
          </thead>
          <tbody>
            {nonTerminals.map((nt, i) => {
              const tokens = sets[nt] ?? [];
              const sorted = [...tokens].sort((a, b) => {
                if (a === 'ε') return -1; if (b === 'ε') return 1;
                if (a === '$') return 1;  if (b === '$') return -1;
                return a.localeCompare(b);
              });
              return (
                <tr key={nt} style={{
                  background: i % 2 === 0 ? theme.bg : theme.bgSecondary,
                  borderBottom: `1px solid ${theme.borderColor}`,
                }}>
                  {/* No-terminal */}
                  <td style={{
                    padding: '8px 16px', fontFamily: 'JetBrains Mono, monospace',
                    fontWeight: 700, color: '#a78bfa', whiteSpace: 'nowrap',
                    borderRight: `2px solid ${accentColor}44`,
                    verticalAlign: 'top',
                  }}>
                    {nt}
                  </td>
                  {/* Conjunto */}
                  <td style={{ padding: '6px 16px' }}>
                    {sorted.length === 0 ? (
                      <span style={{ color: theme.textMuted, fontSize: '12px' }}>∅ vacío</span>
                    ) : (
                      <div style={{ display: 'flex', flexWrap: 'wrap', gap: '6px' }}>
                        {sorted.map(tok => {
                          const isEps  = tok === 'ε';
                          const isEnd  = tok === '$';
                          const bg     = isEps ? '#1a1a00' : isEnd ? '#001a1a' : accentBg;
                          const color  = isEps ? '#fbbf24' : isEnd ? '#22d3ee' : accentColor;
                          const border = isEps ? '#fbbf2444' : isEnd ? '#22d3ee44' : `${accentColor}44`;
                          return (
                            <span key={tok} style={{
                              padding: '3px 10px', borderRadius: '6px',
                              fontFamily: 'JetBrains Mono, monospace',
                              fontSize: '12px', fontWeight: 600,
                              background: bg, color, border: `1px solid ${border}`,
                            }}>
                              {tok}
                            </span>
                          );
                        })}
                        {/* Notación de conjunto */}
                        <span style={{
                          marginLeft: '8px', fontSize: '11px',
                          color: theme.textMuted, fontFamily: theme.fontFamily,
                          alignSelf: 'center',
                        }}>
                          {'{ ' + sorted.join(', ') + ' }'}
                        </span>
                      </div>
                    )}
                  </td>
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>

      {/* Definición formal */}
      <div className="mt-5 rounded-xl p-4"
           style={{ background: theme.bgCard, border: `1px solid ${theme.borderColor}` }}>
        <p style={{ fontSize: '12px', fontWeight: 600, color: theme.textSecondary,
                    marginBottom: '8px', fontFamily: theme.fontFamily }}>
          Definición formal
        </p>
        {kind === 'first' ? (
          <div style={{ fontSize: '12px', color: theme.textMuted,
                        fontFamily: theme.fontFamily, lineHeight: 1.8 }}>
            <p>• Si <code style={{ color: accentColor }}>a</code> es terminal: FIRST(a) = {'{'} a {'}'}</p>
            <p>• Si <code style={{ color: accentColor }}>A → ε</code>: agrega ε a FIRST(A)</p>
            <p>• Si <code style={{ color: accentColor }}>A → X₁ X₂ … Xₙ</code>: agrega FIRST(X₁)−{'{}'}ε{'}'}. Si ε ∈ FIRST(X₁), agrega FIRST(X₂)−{'{}'}ε{'}'}, etc.</p>
            <p>• Si ε ∈ FIRST(Xᵢ) para todo i: agrega ε a FIRST(A)</p>
          </div>
        ) : (
          <div style={{ fontSize: '12px', color: theme.textMuted,
                        fontFamily: theme.fontFamily, lineHeight: 1.8 }}>
            <p>• FOLLOW(S) incluye <code style={{ color: '#22d3ee' }}>$</code> (símbolo inicial)</p>
            <p>• Si <code style={{ color: accentColor }}>A → α B β</code>: agrega FIRST(β)−{'{}'}ε{'}'} a FOLLOW(B)</p>
            <p>• Si <code style={{ color: accentColor }}>A → α B</code> o ε ∈ FIRST(β): agrega FOLLOW(A) a FOLLOW(B)</p>
          </div>
        )}
      </div>
    </div>
  );
}
