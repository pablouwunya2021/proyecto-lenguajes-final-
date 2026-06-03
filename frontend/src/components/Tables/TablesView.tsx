// ============================================================
//  TablesView.tsx — Tablas formales de parsing
//  - Drag-to-pan con mouse en la tabla
//  - Conflictos shift/reduce y reduce/reduce diferenciados
// ============================================================
import { useState, useMemo, useRef, useCallback } from 'react';
import { useStore } from '../../store/useStore';
import type { ModuleTheme } from '../../types/themes';
import type { ResolvedConflict } from '../../types';

interface Props { theme: ModuleTheme }
type TableTab = 'grammar' | 'slr' | 'll1' | 'lalr';

// ── Detecta el tipo de conflicto ─────────────────────────────
function conflictType(value: string): 'sr' | 'rr' | null {
  if (!value.includes('/')) return null;
  const parts = value.split('/');
  const hasShift  = parts.some(p => p.trim().startsWith('s'));
  const hasReduce = parts.some(p => p.trim().startsWith('r'));
  if (hasShift && hasReduce) return 'sr';   // shift-reduce
  if (!hasShift && hasReduce) return 'rr';  // reduce-reduce
  return 'sr'; // fallback
}

// ── Celda de la tabla con colores semánticos ─────────────────
function Cell({ value, theme, resolved }: {
  value: string;
  theme: ModuleTheme;
  resolved?: { resolution: string; description: string };
}) {
  const [hovered, setHovered] = useState(false);
  const conflict = conflictType(value);

  // ── Celda con conflicto resuelto por precedencia ───────────
  // El backend ya eligió la acción ganadora; la marcamos para que
  // sea visible que aquí HUBO un conflicto y cómo se resolvió.
  if (resolved && !conflict && value) {
    const isShift = value.startsWith('s');
    const c = isShift ? '#4488ff' : value.startsWith('r') ? '#ff9944' : '#9ca3af';
    return (
      <td
        title={`✓ Conflicto resuelto por precedencia\n${resolved.description}\nResolución: ${resolved.resolution}`}
        onMouseEnter={() => setHovered(true)}
        onMouseLeave={() => setHovered(false)}
        style={{
          padding: '4px 8px', textAlign: 'center',
          background: hovered ? '#0a2618' : '#08140d',
          color: c,
          fontFamily: 'JetBrains Mono, monospace', fontSize: '11px', fontWeight: 700,
          borderRight: '1px solid #4ade8055',
          borderBottom: '1px solid #4ade8033',
          cursor: 'help', position: 'relative',
        }}
      >
        <span style={{ marginRight: '3px', fontSize: '9px', color: '#4ade80' }}>✓</span>
        {value}
        {hovered && (
          <span style={{
            position: 'absolute', bottom: '100%', left: '50%',
            transform: 'translateX(-50%)', background: '#0a2618',
            border: '1px solid #4ade80', color: '#86efac',
            padding: '4px 8px', borderRadius: '6px', fontSize: '10px',
            whiteSpace: 'nowrap', zIndex: 50, pointerEvents: 'none',
            fontFamily: 'JetBrains Mono, monospace',
          }}>
            resuelto → {resolved.resolution}
          </span>
        )}
      </td>
    );
  }

  if (!value) return (
    <td style={{
      padding: '4px 8px',
      textAlign: 'center',
      color: theme.textMuted,
      fontFamily: 'JetBrains Mono, monospace',
      fontSize: '11px',
      borderRight: `1px solid ${theme.borderColor}`,
    }}>—</td>
  );

  // Colores según tipo
  let bg    = 'transparent';
  let color = theme.textSecondary;
  let border = `1px solid ${theme.borderColor}`;
  let title  = '';

  if (conflict === 'sr') {
    const parts   = value.split('/');
    const winner  = parts.find(p => p.trim().startsWith('s')) ?? parts[0];
    const loser   = parts.find(p => p.trim() !== winner.trim()) ?? parts[1];
    bg     = hovered ? '#3d1a00' : '#2a1200';
    color  = '#ffaa00';
    border = `1px solid #ffaa0088`;
    title  = `⚠ Conflicto Shift-Reduce\nAcciones: ${value}\nGanó: ${winner.trim()} (shift tiene prioridad)\nDescartado: ${loser?.trim()}`;
  } else if (conflict === 'rr') {
    const parts  = value.split('/');
    bg     = hovered ? '#3d0000' : '#280000';
    color  = '#ff4444';
    border = `1px solid #ff444488`;
    title  = `⚠ Conflicto Reduce-Reduce\nAcciones: ${value}\nGanó: ${parts[0].trim()} (primera producción)\nDescartado: ${parts[1]?.trim()}\nIndica ambigüedad real en la gramática`;
  } else if (value === 'acc') {
    bg    = '#003300'; color = '#00ff88';
  } else if (value.startsWith('s')) {
    bg    = '#001833'; color = '#4488ff';
  } else if (value.startsWith('r')) {
    bg    = '#1a0e00'; color = '#ff9944';
  }

  // Para celdas en conflicto, mostrar solo la acción ganadora + indicador
  const displayValue = conflict
    ? value.split('/')[0].trim()
    : value;

  return (
    <td
      title={title}
      onMouseEnter={() => setHovered(true)}
      onMouseLeave={() => setHovered(false)}
      style={{
        padding: '4px 8px',
        textAlign: 'center',
        background: bg,
        color,
        fontFamily: 'JetBrains Mono, monospace',
        fontSize: '11px',
        fontWeight: (value === 'acc' || conflict) ? 700 : 400,
        borderRight: border,
        borderBottom: conflict ? border : `1px solid ${theme.borderColor}`,
        cursor: conflict ? 'help' : 'default',
        boxShadow: conflict === 'rr' && hovered
          ? '0 0 8px #ff444466 inset'
          : conflict === 'sr' && hovered
          ? '0 0 8px #ffaa0055 inset'
          : 'none',
        transition: 'background 0.15s, box-shadow 0.15s',
        position: 'relative',
        outline: conflict && hovered
          ? `2px solid ${conflict === 'rr' ? '#ff4444' : '#ffaa00'}`
          : 'none',
      }}
    >
      {conflict && (
        <span style={{ marginRight: '3px', fontSize: '9px', opacity: 0.8 }}>
          {conflict === 'rr' ? 'RR' : 'SR'}
        </span>
      )}
      {displayValue}
      {conflict && hovered && (
        <span style={{
          position: 'absolute',
          bottom: '100%',
          left: '50%',
          transform: 'translateX(-50%)',
          background: conflict === 'rr' ? '#3d0000' : '#3d1a00',
          border: `1px solid ${conflict === 'rr' ? '#ff4444' : '#ffaa00'}`,
          color: conflict === 'rr' ? '#ff8888' : '#ffcc66',
          padding: '4px 8px',
          borderRadius: '6px',
          fontSize: '10px',
          whiteSpace: 'nowrap',
          zIndex: 50,
          pointerEvents: 'none',
          fontFamily: 'JetBrains Mono, monospace',
        }}>
          {value.split('/').join(' vs ')}
        </span>
      )}
    </td>
  );
}

// ── Cabecera de columna ───────────────────────────────────────
function ColHeader({ label, isTerminal, theme }: {
  label: string; isTerminal: boolean; theme: ModuleTheme;
}) {
  return (
    <th style={{
      padding: '5px 8px',
      textAlign: 'center',
      fontSize: '11px',
      fontFamily: 'JetBrains Mono, monospace',
      fontWeight: 700,
      color: isTerminal ? '#4488ff' : '#44cc44',
      background: theme.bgSecondary,
      borderRight: `1px solid ${theme.borderColor}`,
      borderBottom: `2px solid ${isTerminal ? '#4488ff44' : '#44cc4444'}`,
      whiteSpace: 'nowrap',
    }}>
      {label}
    </th>
  );
}

// ── Hook de drag-to-pan ───────────────────────────────────────
function useDragPan() {
  const ref      = useRef<HTMLDivElement>(null);
  const dragging = useRef(false);
  const startX   = useRef(0);
  const startY   = useRef(0);
  const scrollX  = useRef(0);
  const scrollY  = useRef(0);

  const onMouseDown = useCallback((e: React.MouseEvent) => {
    // Solo botón izquierdo, y solo si no es click en un <td title>
    if (e.button !== 0) return;
    dragging.current = true;
    startX.current   = e.clientX;
    startY.current   = e.clientY;
    scrollX.current  = ref.current?.scrollLeft ?? 0;
    scrollY.current  = ref.current?.scrollTop  ?? 0;
    document.body.style.userSelect = 'none';
    if (ref.current) ref.current.style.cursor = 'grabbing';
  }, []);

  const onMouseMove = useCallback((e: React.MouseEvent) => {
    if (!dragging.current || !ref.current) return;
    const dx = e.clientX - startX.current;
    const dy = e.clientY - startY.current;
    ref.current.scrollLeft = scrollX.current - dx;
    ref.current.scrollTop  = scrollY.current - dy;
  }, []);

  const onMouseUp = useCallback(() => {
    dragging.current = false;
    document.body.style.userSelect = '';
    if (ref.current) ref.current.style.cursor = 'grab';
  }, []);

  const onMouseLeave = useCallback(() => {
    if (dragging.current) {
      dragging.current = false;
      document.body.style.userSelect = '';
      if (ref.current) ref.current.style.cursor = 'grab';
    }
  }, []);

  return { ref, onMouseDown, onMouseMove, onMouseUp, onMouseLeave };
}

// ── Tabla ACTION/GOTO (SLR o LALR) ──────────────────────────
function ActionGotoTable({
  actionEntries, gotoEntries, productions,
  title, conflicts, resolvedConflicts, theme,
}: {
  actionEntries:    { state: number; symbol: string; action: string }[];
  gotoEntries:      { state: number; symbol: string; goto: number }[];
  productions:      { id: number; lhs: string; rhs: string[] }[];
  title:            string;
  conflicts:        any[];
  resolvedConflicts: ResolvedConflict[];
  theme:            ModuleTheme;
}) {
  const pan = useDragPan();

  const { states, terminals, nonTerminals, cells, srCount, rrCount, resolvedMap } = useMemo(() => {
    const stateSet = new Set<number>();
    const termSet  = new Set<string>();
    const ntSet    = new Set<string>();
    const cellMap  = new Map<string, string>();
    // state:symbol → info de resolución por precedencia
    const resolvedMap = new Map<string, { resolution: string; description: string }>();
    for (const r of resolvedConflicts) {
      if (r.state === undefined || !r.symbol) continue;
      resolvedMap.set(`${r.state}:${r.symbol}`, {
        resolution:  r.resolution,
        description: r.description,
      });
      stateSet.add(r.state);
      termSet.add(r.symbol);
    }
    let srCount = 0, rrCount = 0;

    for (const e of actionEntries) {
      stateSet.add(e.state);
      termSet.add(e.symbol);
      const key = `${e.state}:${e.symbol}`;
      if (cellMap.has(key)) {
        cellMap.set(key, cellMap.get(key) + '/' + e.action);
      } else {
        cellMap.set(key, e.action);
      }
    }

    for (const e of gotoEntries) {
      stateSet.add(e.state);
      ntSet.add(e.symbol);
      cellMap.set(`${e.state}:${e.symbol}`, String(e.goto));
    }

    // ── Superponer conflictos resueltos por defecto ──────────────
    // El backend envía solo la acción ganadora en actionEntries.
    // Usamos el array conflicts para reconstruir la celda con ambas
    // acciones (ej: "s53/r39") y así activar el highlighting visual.
    for (const c of conflicts) {
      if (c.state === undefined || !c.symbol || !c.description) continue;
      // Parsear "... : s53 vs r39" o "... : r17 vs r31"
      const match = c.description.match(/:\s*([sr]\d+)\s+vs\s+([sr]\d+)/i);
      if (match) {
        const key = `${c.state}:${c.symbol}`;
        // Solo sobreescribir si la celda no muestra ya un conflicto
        const current = cellMap.get(key) ?? '';
        if (!current.includes('/')) {
          cellMap.set(key, `${match[1]}/${match[2]}`);
        }
      }
      stateSet.add(c.state);
      termSet.add(c.symbol);
    }

    // Contar tipos de conflicto
    for (const v of cellMap.values()) {
      const ct = conflictType(v);
      if (ct === 'sr') srCount++;
      else if (ct === 'rr') rrCount++;
    }

    return {
      states:       Array.from(stateSet).sort((a, b) => a - b),
      terminals:    Array.from(termSet).sort(),
      nonTerminals: Array.from(ntSet).sort(),
      cells:        cellMap,
      srCount,
      rrCount,
      resolvedMap,
    };
  }, [actionEntries, gotoEntries, conflicts, resolvedConflicts]);

  return (
    <div>
      {/* Cabecera de info + badges de conflicto */}
      <div className="flex items-center gap-3 mb-3 flex-wrap">
        <span style={{ fontSize: '12px', color: theme.textMuted, fontFamily: theme.fontFamily }}>
          {title} — {states.length} estados · {terminals.length} terminales · {nonTerminals.length} no-terminales
        </span>

        {/* Leyenda compacta */}
        <div className="flex gap-3 flex-wrap">
          {[
            { label: 'sN = shift',  color: '#4488ff' },
            { label: 'rN = reduce', color: '#ff9944' },
            { label: 'acc',         color: '#00ff88' },
          ].map(l => (
            <span key={l.label} style={{ fontSize: '10px', color: l.color, fontFamily: 'monospace' }}>
              {l.label}
            </span>
          ))}
        </div>

        {/* Badges de conflicto */}
        {srCount > 0 && (
          <span style={{
            fontSize: '11px', fontWeight: 700,
            padding: '2px 10px', borderRadius: '12px',
            background: '#2a1200', color: '#ffaa00',
            border: '1px solid #ffaa0066',
          }}>
            {srCount} SR
          </span>
        )}
        {rrCount > 0 && (
          <span style={{
            fontSize: '11px', fontWeight: 700,
            padding: '2px 10px', borderRadius: '12px',
            background: '#280000', color: '#ff4444',
            border: '1px solid #ff444466',
          }}>
            {rrCount} RR
          </span>
        )}
        {resolvedMap.size > 0 && (
          <span style={{
            fontSize: '11px', fontWeight: 700,
            padding: '2px 10px', borderRadius: '12px',
            background: '#08140d', color: '#4ade80',
            border: '1px solid #4ade8055',
          }}>
            ✓ {resolvedMap.size} resuelto(s) por precedencia
          </span>
        )}
        {srCount === 0 && rrCount === 0 && resolvedMap.size === 0 && (
          <span style={{
            fontSize: '11px', fontWeight: 700,
            padding: '2px 10px', borderRadius: '12px',
            background: '#002200', color: '#00ff88',
            border: '1px solid #00ff8844',
          }}>
            ✓ Sin conflictos
          </span>
        )}
      </div>

      {/* Instrucción de drag */}
      <p style={{ fontSize: '10px', color: theme.textMuted, marginBottom: '6px',
                  fontFamily: theme.fontFamily, fontStyle: 'italic' }}>
        Arrastra con el mouse para navegar · Scroll para desplazar
      </p>

      {/* Tabla con drag-to-pan */}
      <div
        ref={pan.ref}
        onMouseDown={pan.onMouseDown}
        onMouseMove={pan.onMouseMove}
        onMouseUp={pan.onMouseUp}
        onMouseLeave={pan.onMouseLeave}
        style={{
          overflowX: 'auto',
          overflowY: 'auto',
          maxHeight: 'calc(100vh - 340px)',
          cursor: 'grab',
          borderRadius: '8px',
          border: `1px solid ${theme.borderColor}`,
          // Scrollbar delgado y suave
          scrollbarWidth: 'thin',
        }}
      >
        <table style={{
          borderCollapse: 'collapse',
          fontSize: '11px',
          width: 'max-content',
        }}>
          <thead style={{ position: 'sticky', top: 0, zIndex: 2 }}>
            {/* Fila de secciones ACTION / GOTO */}
            <tr>
              <th rowSpan={2} style={{
                padding: '5px 12px',
                background: theme.bgCard,
                color: theme.textPrimary,
                fontFamily: 'JetBrains Mono, monospace',
                fontSize: '12px',
                borderRight: `2px solid ${theme.accentA}`,
                borderBottom: `1px solid ${theme.borderColor}`,
                position: 'sticky',
                left: 0,
                zIndex: 3,
              }}>
                Estado
              </th>
              {terminals.length > 0 && (
                <th colSpan={terminals.length} style={{
                  padding: '4px 8px', textAlign: 'center',
                  background: '#001833', color: '#4488ff',
                  fontSize: '11px', fontWeight: 700,
                  borderRight: `2px solid ${theme.borderColor}`,
                  borderBottom: `1px solid ${theme.borderColor}`,
                }}>
                  ACTION
                </th>
              )}
              {nonTerminals.length > 0 && (
                <th colSpan={nonTerminals.length} style={{
                  padding: '4px 8px', textAlign: 'center',
                  background: '#001a00', color: '#44cc44',
                  fontSize: '11px', fontWeight: 700,
                  borderBottom: `1px solid ${theme.borderColor}`,
                }}>
                  GOTO
                </th>
              )}
            </tr>
            <tr>
              {terminals.map(t => <ColHeader key={t} label={t} isTerminal={true}  theme={theme} />)}
              {nonTerminals.map(nt => <ColHeader key={nt} label={nt} isTerminal={false} theme={theme} />)}
            </tr>
          </thead>

          <tbody>
            {states.map((state, si) => {
              // ¿Tiene esta fila algún conflicto?
              const rowHasSR = terminals.some(t => conflictType(cells.get(`${state}:${t}`) ?? '') === 'sr');
              const rowHasRR = terminals.some(t => conflictType(cells.get(`${state}:${t}`) ?? '') === 'rr');

              return (
                <tr key={state} style={{
                  background: rowHasRR ? '#1a0000'
                            : rowHasSR ? '#160e00'
                            : si % 2 === 0 ? theme.bg : theme.bgSecondary,
                }}>
                  {/* Número de estado — sticky */}
                  <td style={{
                    padding: '4px 12px',
                    textAlign: 'center',
                    fontWeight: 700,
                    fontFamily: 'JetBrains Mono, monospace',
                    fontSize: '11px',
                    color: rowHasRR ? '#ff6666' : rowHasSR ? '#ffbb44' : theme.accentA,
                    background: rowHasRR ? '#200000' : rowHasSR ? '#1e1000' : theme.bgCard,
                    borderRight: `2px solid ${rowHasRR ? '#ff444466' : rowHasSR ? '#ffaa0066' : theme.accentA + '44'}`,
                    borderBottom: `1px solid ${theme.borderColor}`,
                    position: 'sticky',
                    left: 0,
                    zIndex: 1,
                  }}>
                    {state}
                  </td>

                  {terminals.map(t => (
                    <Cell key={t}
                          value={cells.get(`${state}:${t}`) ?? ''}
                          theme={theme}
                          resolved={resolvedMap.get(`${state}:${t}`)} />
                  ))}

                  {nonTerminals.map(nt => (
                    <td key={nt} style={{
                      padding: '4px 8px', textAlign: 'center',
                      color: cells.has(`${state}:${nt}`) ? '#44cc44' : theme.textMuted,
                      fontFamily: 'JetBrains Mono, monospace',
                      fontSize: '11px',
                      borderRight: `1px solid ${theme.borderColor}`,
                      borderBottom: `1px solid ${theme.borderColor}`,
                    }}>
                      {cells.get(`${state}:${nt}`) ?? '—'}
                    </td>
                  ))}
                </tr>
              );
            })}
          </tbody>
        </table>
      </div>

      {/* Producciones */}
      {productions.length > 0 && (
        <div className="mt-4">
          <p style={{ fontSize: '11px', color: theme.textMuted, marginBottom: '6px',
                      fontFamily: theme.fontFamily }}>
            Producciones referenciadas en reduces r(n):
          </p>
          <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(280px, 1fr))', gap: '4px' }}>
            {productions.map(p => (
              <div key={p.id} style={{
                fontSize: '11px', fontFamily: 'JetBrains Mono, monospace',
                color: theme.textSecondary, padding: '3px 8px',
                background: theme.bgCard, borderRadius: '4px',
                border: `1px solid ${theme.borderColor}`,
              }}>
                <span style={{ color: '#ff9944' }}>r{p.id}</span>
                {'  '}
                <span style={{ color: '#44cc44' }}>{p.lhs}</span>
                {' → '}
                <span style={{ color: theme.textPrimary }}>
                  {p.rhs.length === 0 ? 'ε' : p.rhs.join(' ')}
                </span>
              </div>
            ))}
          </div>
        </div>
      )}
    </div>
  );
}

// ── Tabla LL(1) M[NT][T] ─────────────────────────────────────
function LL1Table({
  tableEntries, productions, theme,
}: {
  tableEntries: { nt: string; t: string; prod: number }[];
  productions:  { id: number; lhs: string; rhs: string[] }[];
  theme:        ModuleTheme;
}) {
  const pan = useDragPan();

  const { nonTerminals, terminals, cells, isLL1, conflictCount } = useMemo(() => {
    const ntSet   = new Set<string>();
    const tSet    = new Set<string>();
    const cellMap = new Map<string, string[]>();
    let   conflictCount = 0;

    for (const e of tableEntries) {
      ntSet.add(e.nt); tSet.add(e.t);
      const key = `${e.nt}:${e.t}`;
      if (!cellMap.has(key)) cellMap.set(key, []);
      cellMap.get(key)!.push(`p${e.prod}`);
      if (cellMap.get(key)!.length > 1) conflictCount++;
    }

    const terminals = Array.from(tSet).sort((a, b) => {
      if (a === '$') return 1; if (b === '$') return -1;
      return a.localeCompare(b);
    });

    return {
      nonTerminals: Array.from(ntSet).sort(),
      terminals, cells: cellMap,
      isLL1: conflictCount === 0, conflictCount,
    };
  }, [tableEntries]);

  return (
    <div>
      <div className="flex items-center gap-3 mb-3 flex-wrap">
        <span style={{
          fontSize: '12px', padding: '3px 10px', borderRadius: '12px',
          background: isLL1 ? '#003300' : '#1a0000',
          color:      isLL1 ? '#00ff88' : '#ff4444',
          border:     `1px solid ${isLL1 ? '#00ff8844' : '#ff444444'}`,
          fontFamily: theme.fontFamily, fontWeight: 700,
        }}>
          {isLL1 ? '✓ La gramática ES LL(1)' : `✗ La gramática NO es LL(1) — ${conflictCount} conflicto(s)`}
        </span>
        <span style={{ fontSize: '11px', color: theme.textMuted, fontFamily: theme.fontFamily }}>
          {nonTerminals.length} no-terminales × {terminals.length} terminales
        </span>
      </div>

      {!isLL1 && (
        <div className="mb-3 rounded-lg p-3"
             style={{ background: '#1a0000', border: '2px solid #ff444444' }}>
          <p style={{ fontSize: '11px', color: '#ff8888', fontFamily: theme.fontFamily }}>
            Las celdas marcadas contienen más de una producción — la gramática no es LL(1).
          </p>
        </div>
      )}

      <p style={{ fontSize: '10px', color: theme.textMuted, marginBottom: '6px',
                  fontFamily: theme.fontFamily, fontStyle: 'italic' }}>
        Arrastra con el mouse para navegar · Scroll para desplazar
      </p>

      <div
        ref={pan.ref}
        onMouseDown={pan.onMouseDown}
        onMouseMove={pan.onMouseMove}
        onMouseUp={pan.onMouseUp}
        onMouseLeave={pan.onMouseLeave}
        style={{
          overflowX: 'auto', overflowY: 'auto',
          maxHeight: 'calc(100vh - 340px)',
          cursor: 'grab', borderRadius: '8px',
          border: `1px solid ${theme.borderColor}`,
          scrollbarWidth: 'thin',
        }}
      >
        <table style={{ borderCollapse: 'collapse', fontSize: '11px', width: 'max-content' }}>
          <thead style={{ position: 'sticky', top: 0, zIndex: 2 }}>
            <tr>
              <th style={{
                padding: '6px 14px', background: theme.bgCard, color: theme.textPrimary,
                fontFamily: 'JetBrains Mono, monospace',
                borderRight: `2px solid ${theme.accentA}44`,
                borderBottom: `2px solid ${theme.accentA}44`, fontSize: '11px',
                position: 'sticky', left: 0, zIndex: 3,
              }}>
                NT \ Terminal
              </th>
              {terminals.map(t => (
                <th key={t} style={{
                  padding: '6px 12px', textAlign: 'center',
                  background: t === '$' ? '#1a1a00' : '#001833',
                  color:      t === '$' ? '#ffcc44' : '#4488ff',
                  fontFamily: 'JetBrains Mono, monospace', fontSize: '11px', fontWeight: 700,
                  borderRight: `1px solid ${theme.borderColor}`,
                  borderBottom: `2px solid ${t === '$' ? '#ffcc4444' : '#4488ff44'}`,
                  whiteSpace: 'nowrap',
                }}>
                  {t}
                </th>
              ))}
            </tr>
          </thead>

          <tbody>
            {nonTerminals.map((nt, ni) => (
              <tr key={nt} style={{ background: ni % 2 === 0 ? theme.bg : theme.bgSecondary }}>
                <td style={{
                  padding: '4px 14px', fontWeight: 700,
                  fontFamily: 'JetBrains Mono, monospace', fontSize: '11px', color: '#44cc44',
                  background: theme.bgCard, borderRight: `2px solid ${theme.accentA}44`,
                  borderBottom: `1px solid ${theme.borderColor}`, whiteSpace: 'nowrap',
                  position: 'sticky', left: 0, zIndex: 1,
                }}>
                  {nt}
                </td>
                {terminals.map(t => {
                  const vals       = cells.get(`${nt}:${t}`) ?? [];
                  const isConflict = vals.length > 1;
                  return (
                    <td key={t}
                        title={isConflict ? `🔴 Conflicto LL(1): ${vals.join(' / ')}` : undefined}
                        style={{
                          padding: '4px 12px', textAlign: 'center',
                          fontFamily: 'JetBrains Mono, monospace', fontSize: '11px',
                          background: isConflict ? '#280000' : vals.length > 0 ? '#001422' : 'transparent',
                          color:      isConflict ? '#ff4444' : vals.length > 0 ? '#88aaff' : theme.textMuted,
                          borderRight: `1px solid ${isConflict ? '#ff444488' : theme.borderColor}`,
                          borderBottom: `1px solid ${isConflict ? '#ff444466' : theme.borderColor}`,
                          fontWeight: isConflict ? 700 : 400,
                          cursor: isConflict ? 'help' : 'default',
                        }}>
                      {vals.length > 0 ? vals.join(' / ') : '—'}
                    </td>
                  );
                })}
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      <div className="mt-4">
        <p style={{ fontSize: '11px', color: theme.textMuted, marginBottom: '6px',
                    fontFamily: theme.fontFamily }}>Producciones:</p>
        <div style={{ display: 'grid', gridTemplateColumns: 'repeat(auto-fill, minmax(280px, 1fr))', gap: '4px' }}>
          {productions.map(p => (
            <div key={p.id} style={{
              fontSize: '11px', fontFamily: 'JetBrains Mono, monospace',
              color: theme.textSecondary, padding: '3px 8px',
              background: theme.bgCard, borderRadius: '4px',
              border: `1px solid ${theme.borderColor}`,
            }}>
              <span style={{ color: '#88aaff' }}>p{p.id}</span>{'  '}
              <span style={{ color: '#44cc44' }}>{p.lhs}</span>
              {' → '}
              <span style={{ color: theme.textPrimary }}>
                {p.rhs.length === 0 ? 'ε' : p.rhs.join(' ')}
              </span>
            </div>
          ))}
        </div>
      </div>
    </div>
  );
}

// ── Banner comparativo SLR → LALR ────────────────────────────
// Muestra cómo LALR(1) mejora sobre SLR(1) gracias a lookaheads
// más precisos, y cuántos conflictos resuelve cada uno por precedencia.
function SlrLalrComparison({
  slrConf, slrResolved, lalrConf, lalrResolved, activeTab, theme,
}: {
  slrConf: number; slrResolved: number;
  lalrConf: number; lalrResolved: number;
  activeTab: TableTab; theme: ModuleTheme;
}) {
  // Conflictos que LALR resuelve gracias a lookaheads más precisos
  // (no por precedencia, sino por ser más potente que SLR)
  const improvedByLookahead = Math.max(0, slrConf - lalrConf);

  const Box = ({ label, conf, resolved, highlight }: {
    label: string; conf: number; resolved: number; highlight: boolean;
  }) => (
    <div style={{
      flex: 1, minWidth: '160px',
      padding: '10px 14px', borderRadius: '10px',
      background: highlight ? `${theme.accentA}12` : theme.bgCard,
      border: `1px solid ${highlight ? theme.accentA : theme.borderColor}`,
    }}>
      <div style={{ fontSize: '11px', color: theme.textMuted, marginBottom: '4px', fontFamily: theme.fontFamily }}>
        {label}
      </div>
      <div className="flex items-baseline gap-3">
        <span style={{
          fontSize: '22px', fontWeight: 800, lineHeight: 1,
          fontFamily: 'JetBrains Mono, monospace',
          color: conf > 0 ? '#f87171' : '#4ade80',
        }}>
          {conf}
        </span>
        <span style={{ fontSize: '11px', color: theme.textMuted }}>
          conflicto{conf === 1 ? '' : 's'} sin resolver
        </span>
      </div>
      {resolved > 0 && (
        <div style={{ fontSize: '11px', color: '#4ade80', marginTop: '4px', fontFamily: theme.fontFamily }}>
          ✓ {resolved} resuelto{resolved === 1 ? '' : 's'} por precedencia
        </div>
      )}
    </div>
  );

  return (
    <div className="mb-4 rounded-xl p-4" style={{ background: theme.bgSecondary, border: `1px solid ${theme.borderColor}` }}>
      <p style={{ fontSize: '12px', fontWeight: 700, color: theme.textPrimary, marginBottom: '10px', fontFamily: theme.fontFamily }}>
        Progresión SLR(1) → LALR(1)
      </p>
      <div className="flex items-center gap-3 flex-wrap">
        <Box label="SLR(1)"  conf={slrConf}  resolved={slrResolved}  highlight={activeTab === 'slr'} />
        <span style={{ fontSize: '22px', color: theme.accentA }}>→</span>
        <Box label="LALR(1)" conf={lalrConf} resolved={lalrResolved} highlight={activeTab === 'lalr'} />
      </div>

      {/* Mensaje de mejora */}
      <p style={{ fontSize: '11px', color: theme.textMuted, marginTop: '10px', lineHeight: 1.5, fontFamily: theme.fontFamily }}>
        {improvedByLookahead > 0 ? (
          <>
            <span style={{ color: '#4ade80', fontWeight: 700 }}>
              LALR(1) resolvió {improvedByLookahead} conflicto{improvedByLookahead === 1 ? '' : 's'} más que SLR(1)
            </span>{' '}
            gracias a sus lookaheads más precisos (subconjuntos de FOLLOW calculados por estado).
          </>
        ) : lalrConf === 0 && slrConf === 0 ? (
          <>Ambas tablas están libres de conflictos: la gramática es SLR(1) (y por tanto también LALR(1)).</>
        ) : lalrConf === slrConf && lalrConf > 0 ? (
          <>
            LALR(1) no reduce los conflictos respecto a SLR(1): son conflictos{' '}
            <span style={{ color: '#fbbf24' }}>genuinos</span> que requieren declaraciones de
            precedencia (<code style={{ color: theme.accentA }}>%left</code>/<code style={{ color: theme.accentA }}>%right</code>)
            o reestructurar la gramática.
          </>
        ) : (
          <>El analizador sintáctico usa la tabla LALR(1) por ser la más potente de las dos.</>
        )}
      </p>
    </div>
  );
}

// ── Componente principal ─────────────────────────────────────
export function TablesView({ theme }: Props) {
  const { yaparResult } = useStore();
  const [tab, setTab] = useState<TableTab>('slr');

  const tabs: { id: TableTab; label: string; available: boolean; conflictCount?: number }[] = [
    { id: 'grammar', label: 'Gramática',  available: !!yaparResult },
    { id: 'slr',     label: 'SLR(1)',     available: !!yaparResult,
      conflictCount: (yaparResult?.slr?.conflicts?.length ?? 0) },
    { id: 'll1',     label: 'LL(1)',      available: !!yaparResult },
    { id: 'lalr',    label: 'LALR(1)',    available: !!yaparResult,
      conflictCount: (yaparResult?.lalr?.conflicts?.length ?? 0) },
  ];

  const productions = yaparResult?.grammar?.productions ?? [];

  return (
    <div className="flex flex-col h-full" style={{ fontFamily: theme.fontFamily }}>

      {/* Sub-tabs */}
      <div className="flex items-center gap-1 px-3 py-2 border-b flex-shrink-0"
           style={{ background: theme.bgSecondary, borderColor: theme.borderColor }}>
        {tabs.map(t => {
          const isActive = tab === t.id;
          const hasConflicts = (t.conflictCount ?? 0) > 0;
          return (
            <button
              key={t.id}
              onClick={() => t.available && setTab(t.id)}
              disabled={!t.available}
              className="relative px-3 py-1.5 rounded-lg text-xs font-medium transition-all"
              style={{
                background: isActive ? theme.bgCard : 'transparent',
                border:     `1px solid ${isActive ? theme.accentA : hasConflicts ? '#ffaa0066' : 'transparent'}`,
                color:      !t.available ? theme.textMuted
                          : isActive ? theme.accentA
                          : hasConflicts ? '#ffaa00'
                          : theme.textSecondary,
                cursor:  t.available ? 'pointer' : 'not-allowed',
                opacity: t.available ? 1 : 0.4,
              }}
            >
              {t.label}
              {hasConflicts && (
                <span style={{
                  position: 'absolute', top: '-5px', right: '-5px',
                  background: '#ffaa00', color: '#000',
                  borderRadius: '50%', width: '15px', height: '15px',
                  fontSize: '9px', fontWeight: 700, lineHeight: '15px',
                  textAlign: 'center', display: 'block',
                }}>
                  {t.conflictCount! > 9 ? '9+' : t.conflictCount}
                </span>
              )}
            </button>
          );
        })}
      </div>

      <div className="flex-1 overflow-auto p-4">

        {!yaparResult && (
          <div className="flex items-center justify-center h-full" style={{ color: theme.textMuted }}>
            <p>Ejecuta el análisis YAPar para ver las tablas</p>
          </div>
        )}

        {/* Banner comparativo SLR → LALR (visible en ambos tabs LR) */}
        {yaparResult && (tab === 'slr' || tab === 'lalr') && (
          <SlrLalrComparison
            slrConf={yaparResult.slr?.conflicts?.length ?? 0}
            slrResolved={yaparResult.slr?.resolvedConflicts?.length ?? 0}
            lalrConf={yaparResult.lalr?.conflicts?.length ?? 0}
            lalrResolved={yaparResult.lalr?.resolvedConflicts?.length ?? 0}
            activeTab={tab}
            theme={theme}
          />
        )}

        {/* Gramática */}
        {tab === 'grammar' && yaparResult && (
          <div>
            <p style={{ fontSize: '12px', color: theme.textSecondary, marginBottom: '12px' }}>
              Símbolo inicial:{' '}
              <span style={{ color: '#44cc44', fontFamily: 'monospace', fontWeight: 700 }}>
                {yaparResult.grammar.start}
              </span>
              {' — '}{productions.length} producciones
            </p>

            {(yaparResult.grammar.precedence?.length ?? 0) > 0 && (
              <div className="mb-5 rounded-lg overflow-hidden" style={{ border: '1px solid #a78bfa44' }}>
                <div className="px-3 py-2" style={{ background: '#1a0a2e' }}>
                  <span style={{ fontSize: '12px', color: '#a78bfa', fontWeight: 700 }}>
                    Declaraciones de precedencia
                  </span>
                </div>
                <table style={{ width: '100%', borderCollapse: 'collapse', fontSize: '12px' }}>
                  <thead>
                    <tr style={{ background: '#130820' }}>
                      {['Nivel', 'Tokens', 'Asociatividad'].map(h => (
                        <th key={h} style={{
                          padding: '6px 12px', textAlign: 'left',
                          color: '#9ca3af', fontFamily: 'JetBrains Mono, monospace',
                          borderBottom: `1px solid #a78bfa33`,
                        }}>{h}</th>
                      ))}
                    </tr>
                  </thead>
                  <tbody>
                    {[...new Map(
                      yaparResult.grammar.precedence!.map((p: any) => [p.level, p])
                    ).entries()]
                      .sort(([a], [b]) => (b as number) - (a as number))
                      .map(([level, p]: any) => {
                        const tokensAtLevel = yaparResult.grammar.precedence!
                          .filter((x: any) => x.level === level).map((x: any) => x.token);
                        const assocColor = p.assoc === 'left' ? '#60a5fa'
                                         : p.assoc === 'right' ? '#f472b6' : '#fb923c';
                        return (
                          <tr key={level} style={{ borderBottom: '1px solid #a78bfa22' }}>
                            <td style={{ padding: '5px 12px', fontFamily: 'monospace', color: '#a78bfa', fontWeight: 700 }}>{level}</td>
                            <td style={{ padding: '5px 12px', fontFamily: 'monospace', color: '#e2e8f0' }}>{tokensAtLevel.join('  ')}</td>
                            <td style={{ padding: '5px 12px' }}>
                              <span style={{
                                padding: '2px 8px', borderRadius: '8px', fontSize: '11px',
                                fontFamily: 'monospace', fontWeight: 700,
                                background: `${assocColor}22`, color: assocColor,
                                border: `1px solid ${assocColor}44`,
                              }}>%{p.assoc}</span>
                            </td>
                          </tr>
                        );
                      })}
                  </tbody>
                </table>
              </div>
            )}

            <table style={{
              borderCollapse: 'collapse', fontSize: '12px', width: '100%',
              border: `1px solid ${theme.borderColor}`,
            }}>
              <thead>
                <tr style={{ background: theme.bgCard }}>
                  {['#', 'No-terminal', '→', 'Producción'].map(h => (
                    <th key={h} style={{
                      padding: '6px 12px', textAlign: 'left',
                      color: theme.textSecondary,
                      fontFamily: 'JetBrains Mono, monospace', fontWeight: 600,
                      borderBottom: `2px solid ${theme.accentA}44`,
                      borderRight: `1px solid ${theme.borderColor}`,
                    }}>{h}</th>
                  ))}
                </tr>
              </thead>
              <tbody>
                {productions.map((p: any, i: number) => (
                  <tr key={p.id} style={{
                    background: i % 2 === 0 ? theme.bg : theme.bgSecondary,
                    borderBottom: `1px solid ${theme.borderColor}`,
                  }}>
                    <td style={{ padding: '5px 12px', fontFamily: 'monospace', color: '#ff9944', fontWeight: 700 }}>{p.id}</td>
                    <td style={{ padding: '5px 12px', fontFamily: 'monospace', color: '#44cc44', fontWeight: 600 }}>{p.lhs}</td>
                    <td style={{ padding: '5px 12px', color: theme.textMuted }}>→</td>
                    <td style={{ padding: '5px 12px', fontFamily: 'monospace', color: theme.textPrimary }}>
                      {p.rhs.length === 0 ? <em style={{ color: theme.textMuted }}>ε</em> : p.rhs.join(' ')}
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        )}

        {tab === 'slr' && yaparResult && (
          <ActionGotoTable
            actionEntries={yaparResult.slr?.action ?? []}
            gotoEntries={yaparResult.slr?.goto ?? []}
            productions={productions}
            title="SLR(1)"
            conflicts={yaparResult.slr?.conflicts ?? []}
            resolvedConflicts={yaparResult.slr?.resolvedConflicts ?? []}
            theme={theme}
          />
        )}

        {tab === 'll1' && yaparResult && (
          <LL1Table
            tableEntries={yaparResult.ll1?.table ?? []}
            productions={productions}
            theme={theme}
          />
        )}

        {tab === 'lalr' && yaparResult && (
          <ActionGotoTable
            actionEntries={yaparResult.lalr?.action ?? []}
            gotoEntries={yaparResult.lalr?.goto ?? []}
            productions={productions}
            title="LALR(1)"
            conflicts={yaparResult.lalr?.conflicts ?? []}
            resolvedConflicts={yaparResult.lalr?.resolvedConflicts ?? []}
            theme={theme}
          />
        )}
      </div>
    </div>
  );
}
