// ============================================================
//  AutomatonView.tsx — Visualización del autómata LR(0)
//
//  Muestra únicamente el autómata sintáctico LR(0) del módulo
//  YAPar (el DFA léxico se removió de esta vista).
//
//  Flujo:
//    resultado C++ → campo automaton_dot (string DOT)
//    → POST /graphviz/render → SVG (grafos pequeños, inline)
//    → <div dangerouslySetInnerHTML />
//
//  Grafos grandes (programas grandes → muchos estados):
//    el SVG inline congela el navegador y el render puede dar timeout.
//    En ese caso NO se renderiza en pantalla: se descarga la imagen
//    automáticamente como PNG y se ofrecen botones de descarga.
// ============================================================
import { useEffect, useRef, useState } from 'react';
import { useStore } from '../../store/useStore';
import type { ModuleTheme } from '../../types/themes';

interface Props { theme: ModuleTheme }

const API = 'http://localhost:8000';

// A partir de cuántos estados consideramos el grafo "grande" y
// pasamos al modo descarga en vez de render inline.
const LARGE_STATES = 50;

// ── Descarga una imagen del grafo (svg|png) ─────────────────
async function downloadGraph(dot: string, format: 'svg' | 'png') {
  const res = await fetch(`${API}/graphviz/render`, {
    method:  'POST',
    headers: { 'Content-Type': 'application/json' },
    body:    JSON.stringify({ dot, engine: 'dot', format, download: true }),
  });
  if (!res.ok) {
    const msg = await res.text();
    throw new Error(msg || `Error ${res.status}`);
  }
  const blob = await res.blob();
  const url  = URL.createObjectURL(blob);
  const a    = document.createElement('a');
  a.href     = url;
  a.download = `automata_lr0.${format}`;
  document.body.appendChild(a);
  a.click();
  a.remove();
  URL.revokeObjectURL(url);
}

// ── Hook que convierte DOT a SVG vía API ────────────────────
function useDotSvg(dot: string | null): {
  svg:     string | null;
  loading: boolean;
  error:   string | null;
} {
  const [svg,     setSvg]     = useState<string | null>(null);
  const [loading, setLoading] = useState(false);
  const [error,   setError]   = useState<string | null>(null);

  useEffect(() => {
    if (!dot) { setSvg(null); setError(null); return; }
    setLoading(true);
    setError(null);

    fetch(`${API}/graphviz/render`, {
      method:  'POST',
      headers: { 'Content-Type': 'application/json' },
      body:    JSON.stringify({ dot, engine: 'dot', format: 'svg' }),
    })
      .then(async r => {
        if (!r.ok) throw new Error((await r.text()) || `Error ${r.status}`);
        return r.text();
      })
      .then(text => {
        if (text.includes('<svg')) setSvg(text);
        else setError('Respuesta inesperada del servidor Graphviz');
      })
      .catch(e => setError(e.message))
      .finally(() => setLoading(false));
  }, [dot]);

  return { svg, loading, error };
}

// ── Componente principal ────────────────────────────────────
export function AutomatonView({ theme }: Props) {
  const { yaparResult } = useStore();
  const lr0Dot = yaparResult?.automaton_dot ?? null;
  const states = yaparResult?.automaton?.states?.length ?? 0;
  const isLarge = states > LARGE_STATES;

  // El usuario puede forzar el render inline de un grafo grande.
  const [forceRender, setForceRender] = useState(false);
  const [zoom, setZoom] = useState(1);
  const [dlError, setDlError] = useState<string | null>(null);
  const [dlBusy, setDlBusy] = useState(false);

  // Solo pedimos el SVG inline si el grafo no es grande (o el usuario lo forzó)
  const dotForInline = lr0Dot && (!isLarge || forceRender) ? lr0Dot : null;
  const { svg, loading, error } = useDotSvg(dotForInline);

  // Descarga automática para grafos grandes: una sola vez por cada DOT nuevo.
  const autoDownloadedFor = useRef<string | null>(null);
  useEffect(() => {
    if (lr0Dot && isLarge && !forceRender && autoDownloadedFor.current !== lr0Dot) {
      autoDownloadedFor.current = lr0Dot;
      setDlBusy(true);
      setDlError(null);
      downloadGraph(lr0Dot, 'png')
        .catch(e => setDlError(e.message))
        .finally(() => setDlBusy(false));
    }
  }, [lr0Dot, isLarge, forceRender]);

  // Reset del estado al cambiar de grafo
  useEffect(() => { setForceRender(false); setZoom(1); setDlError(null); }, [lr0Dot]);

  const handleWheel = (e: React.WheelEvent) => {
    e.preventDefault();
    setZoom(z => Math.max(0.2, Math.min(4, z - e.deltaY * 0.001)));
  };

  const runDownload = (format: 'svg' | 'png') => {
    if (!lr0Dot) return;
    setDlBusy(true);
    setDlError(null);
    downloadGraph(lr0Dot, format)
      .catch(e => setDlError(e.message))
      .finally(() => setDlBusy(false));
  };

  // Botones de descarga reutilizables
  const downloadButtons = (
    <div className="flex items-center gap-2">
      <button onClick={() => runDownload('png')} disabled={dlBusy || !lr0Dot}
        className="px-2 py-0.5 rounded text-xs"
        style={{ background: 'transparent', border: `1px solid ${theme.borderColor}`,
                 color: theme.textMuted, cursor: dlBusy ? 'wait' : 'pointer' }}>
        ↓ PNG
      </button>
      <button onClick={() => runDownload('svg')} disabled={dlBusy || !lr0Dot}
        className="px-2 py-0.5 rounded text-xs"
        style={{ background: 'transparent', border: `1px solid ${theme.borderColor}`,
                 color: theme.textMuted, cursor: dlBusy ? 'wait' : 'pointer' }}>
        ↓ SVG
      </button>
    </div>
  );

  return (
    <div className="flex flex-col h-full" style={{ fontFamily: theme.fontFamily }}>
      {/* Header */}
      <div
        className="flex items-center justify-between px-4 py-2 border-b text-xs flex-shrink-0"
        style={{ background: theme.bgSecondary, borderColor: theme.borderColor }}
      >
        <div className="flex items-center gap-3">
          <span style={{ color: theme.textPrimary, fontWeight: 600 }}>Autómata LR(0)</span>
          {states > 0 && (
            <span style={{ color: theme.textMuted }}>{states} estados</span>
          )}
        </div>
        <div className="flex items-center gap-3">
          {svg && (
            <>
              <span style={{ color: theme.textMuted }}>Zoom: {Math.round(zoom * 100)}%</span>
              <button onClick={() => setZoom(1)} className="px-2 py-0.5 rounded text-xs"
                style={{ background: 'transparent', border: `1px solid ${theme.borderColor}`, color: theme.textMuted, cursor: 'pointer' }}>
                100%
              </button>
              <button onClick={() => setZoom(z => Math.min(4, z + 0.25))} className="px-2 py-0.5 rounded text-xs"
                style={{ background: 'transparent', border: `1px solid ${theme.borderColor}`, color: theme.textMuted, cursor: 'pointer' }}>
                +
              </button>
              <button onClick={() => setZoom(z => Math.max(0.2, z - 0.25))} className="px-2 py-0.5 rounded text-xs"
                style={{ background: 'transparent', border: `1px solid ${theme.borderColor}`, color: theme.textMuted, cursor: 'pointer' }}>
                −
              </button>
            </>
          )}
          {lr0Dot && downloadButtons}
        </div>
      </div>

      {/* Contenido */}
      <div
        className="flex-1 overflow-auto flex items-start justify-start"
        onWheel={svg ? handleWheel : undefined}
        style={{ background: theme.bg, cursor: svg ? 'zoom-in' : 'default', padding: '12px' }}
      >
        {/* Grafo grande → modo descarga */}
        {lr0Dot && isLarge && !forceRender && (
          <div className="m-auto mt-16 text-center px-6 max-w-md">
            <p className="text-sm" style={{ color: theme.textPrimary, fontWeight: 600 }}>
              Autómata demasiado grande para mostrarse en pantalla
            </p>
            <p className="text-xs mt-2" style={{ color: theme.textMuted }}>
              Con {states} estados, renderizarlo en el navegador lo congelaría.
              {dlBusy
                ? ' Descargando la imagen PNG...'
                : dlError
                  ? ''
                  : ' La imagen PNG se descargó automáticamente.'}
            </p>

            {dlError && (
              <p className="text-xs mt-2 font-mono" style={{ color: theme.accentB }}>
                Error al descargar: {dlError}
              </p>
            )}

            <div className="flex items-center justify-center gap-2 mt-4">
              <button onClick={() => runDownload('png')} disabled={dlBusy}
                className="px-3 py-1 rounded text-xs"
                style={{ background: theme.accentA, color: theme.bg, border: 'none',
                         cursor: dlBusy ? 'wait' : 'pointer', fontWeight: 600 }}>
                {dlBusy ? 'Descargando…' : 'Descargar PNG'}
              </button>
              <button onClick={() => runDownload('svg')} disabled={dlBusy}
                className="px-3 py-1 rounded text-xs"
                style={{ background: 'transparent', border: `1px solid ${theme.borderColor}`,
                         color: theme.textMuted, cursor: dlBusy ? 'wait' : 'pointer' }}>
                Descargar SVG
              </button>
            </div>

            <button onClick={() => setForceRender(true)}
              className="mt-4 text-xs underline"
              style={{ background: 'transparent', border: 'none', color: theme.textMuted, cursor: 'pointer' }}>
              Mostrar en pantalla de todas formas (puede congelar el navegador)
            </button>
          </div>
        )}

        {loading && (
          <div className="flex items-center gap-3 m-auto mt-20" style={{ color: theme.textMuted }}>
            <div className="w-4 h-4 border border-current border-t-transparent rounded-full animate-spin" />
            <span>Renderizando con Graphviz...</span>
          </div>
        )}

        {error && (
          <div className="m-auto mt-20 text-center px-6" style={{ color: theme.accentB }}>
            <p className="text-sm font-mono">Error: {error}</p>
            <p className="text-xs mt-2" style={{ color: theme.textMuted }}>
              El grafo puede ser demasiado grande. Prueba a descargarlo:
            </p>
            <div className="flex items-center justify-center gap-2 mt-3">
              <button onClick={() => runDownload('png')} disabled={dlBusy}
                className="px-3 py-1 rounded text-xs"
                style={{ background: theme.accentA, color: theme.bg, border: 'none',
                         cursor: dlBusy ? 'wait' : 'pointer', fontWeight: 600 }}>
                Descargar PNG
              </button>
              <button onClick={() => runDownload('svg')} disabled={dlBusy}
                className="px-3 py-1 rounded text-xs"
                style={{ background: 'transparent', border: `1px solid ${theme.borderColor}`,
                         color: theme.textMuted, cursor: dlBusy ? 'wait' : 'pointer' }}>
                Descargar SVG
              </button>
            </div>
          </div>
        )}

        {!loading && !error && !lr0Dot && (
          <div className="m-auto mt-20 text-center" style={{ color: theme.textMuted }}>
            <p className="text-sm">Ejecuta el análisis YAPar para ver el autómata LR(0)</p>
          </div>
        )}

        {svg && (
          <div
            style={{ transform: `scale(${zoom})`, transformOrigin: 'top left', transition: 'transform 0.1s ease' }}
            dangerouslySetInnerHTML={{ __html: svg }}
          />
        )}
      </div>
    </div>
  );
}
