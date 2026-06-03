// ============================================================
//  ConsolePanel.tsx — Panel de consola profesional inferior
//  Muestra errores, advertencias, info y éxitos estructurados.
//  Resizable arrastrando el borde superior.
// ============================================================
import { useEffect, useRef, useState, useCallback } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { useStore } from '../../store/useStore';
import type { ModuleTheme } from '../../types/themes';
import type { ConsoleEntry, LogLevel, LogSource } from '../../types/console';

interface Props {
  theme:        ModuleTheme;
  onGoToLine?:  (file: 'yalex' | 'yapar' | 'input', line: number, col: number) => void;
}

// ── Configuración visual por nivel ───────────────────────────
const LEVEL_CONFIG: Record<LogLevel, { icon: string; color: string; bg: string }> = {
  error:   { icon: '✕', color: '#f87171', bg: '#7f1d1d22' },
  warning: { icon: '⚠', color: '#fbbf24', bg: '#78350f22' },
  info:    { icon: 'ℹ', color: '#60a5fa', bg: '#1e3a5f22' },
  success: { icon: '✓', color: '#4ade80', bg: '#14532d22' },
};

const SOURCE_COLORS: Record<LogSource, string> = {
  YALex:   '#a78bfa',
  YAPar:   '#fb923c',
  Sistema: '#94a3b8',
  API:     '#f472b6',
};

function formatTime(ts: number): string {
  const d = new Date(ts);
  return `${String(d.getHours()).padStart(2,'0')}:${String(d.getMinutes()).padStart(2,'0')}:${String(d.getSeconds()).padStart(2,'0')}.${String(d.getMilliseconds()).padStart(3,'0')}`;
}

function EntryRow({
  entry,
  theme,
  onGoToLine,
}: {
  entry:       ConsoleEntry;
  theme:       ModuleTheme;
  onGoToLine?: Props['onGoToLine'];
}) {
  const cfg     = LEVEL_CONFIG[entry.level];
  const hasLoc  = entry.line !== undefined;
  const [open, setOpen] = useState(false);

  return (
    <div
      className="flex flex-col border-b"
      style={{
        borderColor: `${theme.borderColor}55`,
        background:  open ? cfg.bg : 'transparent',
        transition:  'background 0.15s',
      }}
    >
      <div
        className="flex items-center gap-2 px-3 py-1.5 text-xs cursor-pointer group"
        style={{ minHeight: '28px' }}
        onClick={() => entry.detail && setOpen(o => !o)}
      >
        {/* Nivel icon */}
        <span
          style={{
            color:      cfg.color,
            fontSize:   '11px',
            fontWeight: 700,
            width:      '14px',
            flexShrink: 0,
            textAlign:  'center',
          }}
        >
          {cfg.icon}
        </span>

        {/* Timestamp */}
        <span style={{ color: theme.textMuted, fontFamily: 'JetBrains Mono, monospace', fontSize: '10px', flexShrink: 0 }}>
          {formatTime(entry.timestamp)}
        </span>

        {/* Source badge */}
        <span
          className="px-1.5 py-0.5 rounded-sm"
          style={{
            background: `${SOURCE_COLORS[entry.source]}22`,
            border:     `1px solid ${SOURCE_COLORS[entry.source]}55`,
            color:      SOURCE_COLORS[entry.source],
            fontFamily: 'JetBrains Mono, monospace',
            fontSize:   '10px',
            fontWeight: 600,
            flexShrink: 0,
          }}
        >
          {entry.source}
        </span>

        {/* Mensaje */}
        <span
          style={{
            color:      entry.level === 'error' ? cfg.color : theme.textPrimary,
            fontFamily: 'JetBrains Mono, monospace',
            fontSize:   '11px',
            flex:       1,
            overflow:   'hidden',
            textOverflow: 'ellipsis',
            whiteSpace: 'nowrap',
          }}
        >
          {entry.message}
        </span>

        {/* Ubicación clickeable */}
        {hasLoc && (
          <button
            onClick={(e) => {
              e.stopPropagation();
              onGoToLine?.(entry.file ?? 'yalex', entry.line!, entry.col ?? 1);
            }}
            className="flex-shrink-0 px-1.5 py-0.5 rounded transition-all"
            style={{
              background: `${cfg.color}15`,
              border:     `1px solid ${cfg.color}40`,
              color:      cfg.color,
              fontFamily: 'JetBrains Mono, monospace',
              fontSize:   '10px',
              cursor:     'pointer',
              whiteSpace: 'nowrap',
            }}
            onMouseEnter={e => (e.currentTarget.style.background = `${cfg.color}30`)}
            onMouseLeave={e => (e.currentTarget.style.background = `${cfg.color}15`)}
          >
            línea {entry.line}{entry.col !== undefined ? `:${entry.col}` : ''}
          </button>
        )}

        {/* Chevron si hay detalle */}
        {entry.detail && (
          <span style={{ color: theme.textMuted, fontSize: '10px', flexShrink: 0 }}>
            {open ? '▾' : '▸'}
          </span>
        )}
      </div>

      {/* Detalle expandible */}
      <AnimatePresence>
        {open && entry.detail && (
          <motion.div
            initial={{ height: 0, opacity: 0 }}
            animate={{ height: 'auto', opacity: 1 }}
            exit={{ height: 0, opacity: 0 }}
            transition={{ duration: 0.15 }}
            style={{ overflow: 'hidden' }}
          >
            <div
              className="px-10 pb-2 text-xs"
              style={{
                fontFamily: 'JetBrains Mono, monospace',
                color:      theme.textSecondary,
                borderLeft: `2px solid ${cfg.color}44`,
                marginLeft: '22px',
                whiteSpace: 'pre-wrap',
              }}
            >
              {entry.detail}
            </div>
          </motion.div>
        )}
      </AnimatePresence>
    </div>
  );
}

// ── Panel principal ──────────────────────────────────────────
const MIN_HEIGHT = 32;
const DEFAULT_HEIGHT = 220;
const MAX_HEIGHT_RATIO = 0.6;

export function ConsolePanel({ theme, onGoToLine }: Props) {
  const {
    consoleEntries,
    consoleOpen,
    setConsoleOpen,
    clearConsole,
  } = useStore();

  const [height,    setHeight]    = useState(DEFAULT_HEIGHT);
  const [filter,    setFilter]    = useState<LogLevel | 'all'>('all');
  const [dragging,  setDragging]  = useState(false);
  const bodyRef    = useRef<HTMLDivElement>(null);
  const dragStartY = useRef(0);
  const dragStartH = useRef(0);

  // Auto-scroll al último entry
  useEffect(() => {
    if (consoleOpen && bodyRef.current) {
      bodyRef.current.scrollTop = bodyRef.current.scrollHeight;
    }
  }, [consoleEntries, consoleOpen]);

  // Drag resize
  const onMouseDown = useCallback((e: React.MouseEvent) => {
    e.preventDefault();
    dragStartY.current = e.clientY;
    dragStartH.current = height;
    setDragging(true);
  }, [height]);

  useEffect(() => {
    if (!dragging) return;
    const onMove = (e: MouseEvent) => {
      const delta   = dragStartY.current - e.clientY;
      const maxH    = window.innerHeight * MAX_HEIGHT_RATIO;
      const newH    = Math.min(maxH, Math.max(MIN_HEIGHT, dragStartH.current + delta));
      setHeight(newH);
      if (newH > MIN_HEIGHT + 10) setConsoleOpen(true);
    };
    const onUp = () => setDragging(false);
    window.addEventListener('mousemove', onMove);
    window.addEventListener('mouseup', onUp);
    return () => {
      window.removeEventListener('mousemove', onMove);
      window.removeEventListener('mouseup', onUp);
    };
  }, [dragging, setConsoleOpen]);

  const filtered = filter === 'all'
    ? consoleEntries
    : consoleEntries.filter(e => e.level === filter);

  const counts = {
    error:   consoleEntries.filter(e => e.level === 'error').length,
    warning: consoleEntries.filter(e => e.level === 'warning').length,
    info:    consoleEntries.filter(e => e.level === 'info').length,
    success: consoleEntries.filter(e => e.level === 'success').length,
  };

  const hasErrors   = counts.error   > 0;
  const hasWarnings = counts.warning > 0;
  const isTerminal  = theme.scanlines;

  return (
    <div
      className="flex flex-col border-t flex-shrink-0 relative"
      style={{
        height:      consoleOpen ? height : MIN_HEIGHT,
        borderColor: hasErrors ? '#ef444466' : hasWarnings ? '#f59e0b66' : theme.borderColor,
        background:  theme.bgSecondary,
        transition:  dragging ? 'none' : 'height 0.2s ease',
        userSelect:  dragging ? 'none' : 'auto',
      }}
    >
      {/* Drag handle */}
      <div
        onMouseDown={onMouseDown}
        style={{
          position:   'absolute',
          top:        0,
          left:       0,
          right:      0,
          height:     '4px',
          cursor:     'ns-resize',
          zIndex:     10,
          background: dragging ? theme.accentA : 'transparent',
          transition: 'background 0.15s',
        }}
        onMouseEnter={e => { if (!dragging) (e.currentTarget as HTMLElement).style.background = `${theme.accentA}44`; }}
        onMouseLeave={e => { if (!dragging) (e.currentTarget as HTMLElement).style.background = 'transparent'; }}
      />

      {/* Header */}
      <div
        className="flex items-center gap-2 px-3 flex-shrink-0"
        style={{
          height:       `${MIN_HEIGHT}px`,
          borderBottom: consoleOpen ? `1px solid ${theme.borderColor}` : 'none',
        }}
      >
        {/* Título */}
        <button
          onClick={() => setConsoleOpen(!consoleOpen)}
          className="flex items-center gap-1.5"
          style={{
            background: 'none',
            border:     'none',
            color:      hasErrors ? '#f87171' : hasWarnings ? '#fbbf24' : theme.textMuted,
            fontFamily: theme.fontFamily,
            fontSize:   '11px',
            fontWeight: 600,
            cursor:     'pointer',
            letterSpacing: isTerminal ? '0.08em' : 'normal',
          }}
        >
          <span style={{ fontSize: '10px' }}>{consoleOpen ? '▾' : '▸'}</span>
          {isTerminal ? '[ CONSOLA ]' : 'CONSOLA'}
        </button>

        {/* Badges de conteo */}
        {counts.error > 0 && (
          <button
            onClick={() => setFilter(filter === 'error' ? 'all' : 'error')}
            className="flex items-center gap-1 px-1.5 py-0.5 rounded"
            style={{
              background: filter === 'error' ? '#f8717133' : '#f8717118',
              border:     `1px solid ${filter === 'error' ? '#f87171' : '#f8717144'}`,
              color:      '#f87171',
              fontSize:   '10px',
              fontWeight: 700,
              cursor:     'pointer',
              fontFamily: 'JetBrains Mono, monospace',
            }}
          >
            ✕ {counts.error}
          </button>
        )}
        {counts.warning > 0 && (
          <button
            onClick={() => setFilter(filter === 'warning' ? 'all' : 'warning')}
            className="flex items-center gap-1 px-1.5 py-0.5 rounded"
            style={{
              background: filter === 'warning' ? '#fbbf2433' : '#fbbf2418',
              border:     `1px solid ${filter === 'warning' ? '#fbbf24' : '#fbbf2444'}`,
              color:      '#fbbf24',
              fontSize:   '10px',
              fontWeight: 700,
              cursor:     'pointer',
              fontFamily: 'JetBrains Mono, monospace',
            }}
          >
            ⚠ {counts.warning}
          </button>
        )}
        {counts.success > 0 && counts.error === 0 && (
          <span
            className="flex items-center gap-1 px-1.5 py-0.5 rounded"
            style={{
              background: '#4ade8018',
              border:     '1px solid #4ade8044',
              color:      '#4ade80',
              fontSize:   '10px',
              fontWeight: 700,
              fontFamily: 'JetBrains Mono, monospace',
            }}
          >
            ✓ {counts.success}
          </span>
        )}

        <div className="flex-1" />

        {/* Botón limpiar */}
        {consoleEntries.length > 0 && (
          <button
            onClick={clearConsole}
            title="Limpiar consola"
            style={{
              background: 'none',
              border:     `1px solid ${theme.borderColor}`,
              color:      theme.textMuted,
              borderRadius: '4px',
              padding:    '1px 6px',
              fontSize:   '10px',
              cursor:     'pointer',
              fontFamily: theme.fontFamily,
            }}
            onMouseEnter={e => {
              (e.currentTarget as HTMLElement).style.borderColor = theme.accentB;
              (e.currentTarget as HTMLElement).style.color = theme.accentB;
            }}
            onMouseLeave={e => {
              (e.currentTarget as HTMLElement).style.borderColor = theme.borderColor;
              (e.currentTarget as HTMLElement).style.color = theme.textMuted;
            }}
          >
            {isTerminal ? '[ limpiar ]' : 'limpiar'}
          </button>
        )}
      </div>

      {/* Cuerpo — lista de entries */}
      {consoleOpen && (
        <div
          ref={bodyRef}
          className="flex-1 overflow-y-auto overflow-x-hidden"
          style={{ background: theme.bg }}
        >
          {filtered.length === 0 ? (
            <div
              className="flex items-center justify-center h-full text-xs"
              style={{ color: theme.textMuted, fontFamily: 'JetBrains Mono, monospace' }}
            >
              {isTerminal
                ? '// sin entradas en la consola'
                : 'Sin entradas. Ejecuta el análisis para ver resultados.'}
            </div>
          ) : (
            filtered.map(entry => (
              <EntryRow
                key={entry.id}
                entry={entry}
                theme={theme}
                onGoToLine={onGoToLine}
              />
            ))
          )}
        </div>
      )}
    </div>
  );
}
