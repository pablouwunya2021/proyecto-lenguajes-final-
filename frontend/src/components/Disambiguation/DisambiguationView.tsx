// ============================================================
//  DisambiguationView.tsx — Gramática antes y después
//
//  Muestra dos paneles lado a lado:
//   IZQUIERDA — gramática original tal como se escribió (ambigua
//               si tiene producciones de la forma E → E OP E).
//   DERECHA   — gramática equivalente con la ambigüedad removida,
//               estratificada según las precedencias declaradas.
// ============================================================
import { useMemo } from 'react';
import { useStore } from '../../store/useStore';
import type { ModuleTheme } from '../../types/themes';
import type { PrecDecl } from '../../types';

interface Props { theme: ModuleTheme }

type Prod = { id: number; lhs: string; rhs: string[] };

function assocColor(assoc: string) {
  if (assoc === 'left')     return '#60a5fa';
  if (assoc === 'right')    return '#f472b6';
  if (assoc === 'nonassoc') return '#fb923c';
  return '#9ca3af';
}

// ── Bloque que renderiza una gramática agrupada por LHS ───────
function GrammarBlock({
  groups, theme, ambiguousIds, accent,
}: {
  groups:       { lhs: string; alts: { rhs: string; comment?: string; ambiguous?: boolean }[] }[];
  theme:        ModuleTheme;
  ambiguousIds?: Set<string>;
  accent:       string;
}) {
  return (
    <div className="p-4" style={{ background: theme.bgCard, fontFamily: 'JetBrains Mono, monospace' }}>
      {groups.map(g => (
        <div key={g.lhs} className="mb-3">
          {g.alts.map((alt, i) => {
            const isAmbig = alt.ambiguous ||
              (ambiguousIds?.has(`${g.lhs}→${alt.rhs}`) ?? false);
            return (
              <div key={i} className="flex items-baseline gap-2 text-sm leading-6">
                <span style={{ minWidth: '110px', color: accent, fontWeight: 600 }}>
                  {i === 0 ? g.lhs : ''}
                </span>
                <span style={{ color: theme.textMuted }}>{i === 0 ? ':' : '|'}</span>
                <span style={{
                  color: isAmbig ? '#fbbf24' : theme.textPrimary,
                  background: isAmbig ? '#78350f33' : 'transparent',
                  padding: isAmbig ? '0 4px' : 0, borderRadius: '3px',
                }}>
                  {alt.rhs || 'ε'}
                </span>
                {alt.comment && (
                  <span style={{ color: theme.textMuted, fontSize: '11px' }}>
                    {'  (* '}{alt.comment}{' *)'}
                  </span>
                )}
                {isAmbig && (
                  <span style={{ color: '#fbbf24', fontSize: '10px', fontWeight: 700 }}>
                    ◄ ambigua
                  </span>
                )}
              </div>
            );
          })}
        </div>
      ))}
    </div>
  );
}

// ── Genera la gramática equivalente estratificada ─────────────
function buildEquivalent(productions: Prod[], precedence: PrecDecl[]) {
  if (precedence.length === 0) return null;

  const levelMap = new Map<number, PrecDecl[]>();
  for (const p of precedence) {
    if (!p) continue;
    if (!levelMap.has(p.level)) levelMap.set(p.level, []);
    levelMap.get(p.level)!.push(p);
  }
  const levels = Array.from(levelMap.entries()).sort(([a], [b]) => b - a);
  if (levels.length === 0) return null;

  const lhsSet = new Set(productions.map(p => p.lhs));
  // Producciones ambiguas: LHS → LHS OP LHS
  const ambigProds = productions.filter(p =>
    p.rhs.length === 3 && p.rhs[0] === p.lhs && p.rhs[2] === p.lhs &&
    p.rhs[1] && !lhsSet.has(p.rhs[1])
  );
  const baseLHS = ambigProds.length > 0 ? ambigProds[0].lhs : null;
  if (!baseLHS) return null;

  const ambigTokens = new Set(ambigProds.map(p => p.rhs[1]));
  const atomicProds = productions.filter(p =>
    p.lhs === baseLHS && !ambigTokens.has(p.rhs[1] ?? '')
  );

  const ntNames: string[] = [];
  levels.forEach((_, i) => ntNames.push(i === 0 ? baseLHS : `${baseLHS}_t${i}`));
  const atomNT = `${baseLHS}_atom`;

  const groups: { lhs: string; alts: { rhs: string; comment?: string }[] }[] = [];

  for (let i = 0; i < levels.length; i++) {
    const [, decls] = levels[i];
    const curNT  = ntNames[i];
    const nextNT = ntNames[i + 1] ?? atomNT;
    const assoc  = decls[0].assoc;
    const ops    = decls.map(d => d.token);
    const alts: { rhs: string; comment?: string }[] = [];
    for (const op of ops) {
      if (assoc === 'left')       alts.push({ rhs: `${curNT} ${op} ${nextNT}`, comment: `%left ${op}` });
      else if (assoc === 'right') alts.push({ rhs: `${nextNT} ${op} ${curNT}`, comment: `%right ${op}` });
      else                        alts.push({ rhs: `${nextNT} ${op} ${nextNT}`, comment: `%nonassoc ${op}` });
    }
    alts.push({ rhs: nextNT });
    groups.push({ lhs: curNT, alts });
  }

  const atomAlts = atomicProds.map(ap => ({ rhs: ap.rhs.join(' ') || 'ε' }));
  groups.push({ lhs: atomNT, alts: atomAlts.length ? atomAlts : [{ rhs: 'ε' }] });

  return { groups, ambigTokens, baseLHS };
}

// ── Componente principal ──────────────────────────────────────
export function DisambiguationView({ theme }: Props) {
  const { yaparResult } = useStore();

  if (!yaparResult) {
    return (
      <div className="flex items-center justify-center h-full" style={{ color: theme.textMuted }}>
        <div className="text-center">
          <div className="text-5xl mb-4">⚖</div>
          <p className="text-lg font-medium" style={{ color: theme.textPrimary }}>
            Gramática antes y después
          </p>
          <p className="text-sm mt-2" style={{ fontFamily: theme.fontFamily }}>
            Ejecuta el análisis con un .yapar para ver cómo cambia la gramática
          </p>
        </div>
      </div>
    );
  }

  const productions = (yaparResult.grammar?.productions ?? []) as Prod[];
  const precedence  = yaparResult.grammar?.precedence ?? [];
  const conflicts   = yaparResult.slr?.conflicts ?? [];

  // Gramática original agrupada por LHS
  const originalGroups = useMemo(() => {
    const map = new Map<string, { rhs: string; ambiguous?: boolean }[]>();
    for (const p of productions) {
      const isAmbig = p.rhs.length === 3 && p.rhs[0] === p.lhs && p.rhs[2] === p.lhs;
      if (!map.has(p.lhs)) map.set(p.lhs, []);
      map.get(p.lhs)!.push({ rhs: p.rhs.join(' '), ambiguous: isAmbig });
    }
    return Array.from(map.entries()).map(([lhs, alts]) => ({ lhs, alts }));
  }, [productions]);

  const equivalent = useMemo(
    () => buildEquivalent(productions, precedence),
    [productions, precedence]
  );

  const hasAmbiguity = originalGroups.some(g => g.alts.some(a => a.ambiguous));

  return (
    <div className="h-full overflow-auto p-5" style={{ fontFamily: theme.fontFamily }}>
      {/* Título */}
      <div className="mb-4">
        <h2 className="text-base font-bold mb-1" style={{ color: theme.textPrimary }}>
          Gramática antes y después de la desambiguación
        </h2>
        <p style={{ fontSize: '12px', color: theme.textMuted }}>
          A la izquierda, la gramática tal como se escribió. A la derecha, la versión
          equivalente con la ambigüedad eliminada usando las precedencias declaradas.
        </p>
      </div>

      {/* Estado / resumen breve */}
      <div className="mb-4 flex flex-wrap gap-2">
        {hasAmbiguity ? (
          <span className="px-3 py-1 rounded-lg text-xs font-semibold"
                style={{ background: '#78350f33', border: '1px solid #fbbf2455', color: '#fbbf24' }}>
            ⚠ La gramática original tiene producciones ambiguas (E → E OP E)
          </span>
        ) : (
          <span className="px-3 py-1 rounded-lg text-xs font-semibold"
                style={{ background: '#14532d33', border: '1px solid #4ade8055', color: '#4ade80' }}>
            ✓ La gramática original no tiene ambigüedad de operadores
          </span>
        )}
        {conflicts.length > 0 && (
          <span className="px-3 py-1 rounded-lg text-xs font-semibold"
                style={{ background: '#7f1d1d33', border: '1px solid #f8717155', color: '#f87171' }}>
            {conflicts.length} conflicto(s) sin resolver
          </span>
        )}
      </div>

      {/* Paneles antes / después */}
      <div className="grid gap-4" style={{ gridTemplateColumns: '1fr 1fr' }}>

        {/* ANTES */}
        <div className="rounded-xl overflow-hidden" style={{ border: `1px solid ${theme.borderColor}` }}>
          <div className="px-4 py-2 flex items-center gap-2"
               style={{ background: theme.bgSecondary, borderBottom: `1px solid ${theme.borderColor}` }}>
            <span style={{ fontSize: '12px', fontWeight: 700, color: theme.textPrimary }}>
              Antes (original)
            </span>
            <span style={{ fontSize: '11px', color: theme.textMuted }}>
              {productions.length} producciones
            </span>
          </div>
          <GrammarBlock groups={originalGroups} theme={theme} accent="#fbbf24" />
        </div>

        {/* DESPUÉS */}
        <div className="rounded-xl overflow-hidden" style={{ border: `1px solid #4ade8044` }}>
          <div className="px-4 py-2 flex items-center gap-2"
               style={{ background: '#0a1f12', borderBottom: '1px solid #4ade8033' }}>
            <span style={{ fontSize: '12px', fontWeight: 700, color: '#4ade80' }}>
              Después (sin ambigüedad)
            </span>
          </div>

          {equivalent ? (
            <GrammarBlock groups={equivalent.groups} theme={theme} accent="#4ade80" />
          ) : hasAmbiguity ? (
            <div className="p-4">
              <p style={{ fontSize: '12px', color: theme.textMuted, lineHeight: 1.6 }}>
                La gramática tiene ambigüedad pero{' '}
                <span style={{ color: '#fbbf24' }}>no hay declaraciones de precedencia</span>{' '}
                (<code style={{ color: theme.accentA }}>%left</code>,{' '}
                <code style={{ color: theme.accentA }}>%right</code>) para generar
                automáticamente la versión estratificada.
                <br /><br />
                Agrega precedencias antes del <code style={{ color: theme.accentA }}>%%</code> para
                ver aquí la transformación.
              </p>
            </div>
          ) : (
            <div className="p-4">
              <p style={{ fontSize: '12px', color: theme.textMuted, lineHeight: 1.6 }}>
                La gramática no es ambigua, así que{' '}
                <span style={{ color: '#4ade80' }}>no requiere ninguna transformación</span>:
                la versión "después" es idéntica a la original.
              </p>
            </div>
          )}
        </div>
      </div>

      {/* Nota explicativa cuando sí hubo transformación */}
      {equivalent && (
        <p className="mt-4" style={{ fontSize: '11px', color: theme.textMuted, lineHeight: 1.6 }}>
          La gramática de la derecha estratifica el no-terminal{' '}
          <code style={{ color: '#4ade80' }}>{equivalent.baseLHS}</code> en varios niveles, uno por cada
          nivel de precedencia. Así la precedencia y la asociatividad quedan codificadas en la
          estructura de la gramática y desaparecen los conflictos shift/reduce — misma semántica,
          0 ambigüedad. Las precedencias se colorean según su tipo:{' '}
          <span style={{ color: assocColor('left') }}>%left</span>,{' '}
          <span style={{ color: assocColor('right') }}>%right</span>,{' '}
          <span style={{ color: assocColor('nonassoc') }}>%nonassoc</span>.
        </p>
      )}
    </div>
  );
}
