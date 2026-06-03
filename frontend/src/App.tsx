// ============================================================
//  App.tsx — IDE unificado, una sola pantalla
//  Carga .yalex, .yapar y .txt en el mismo panel.
//  Tema cambia automáticamente al detectar el lenguaje.
// ============================================================
import { useEffect, useState, useCallback, useMemo } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { useStore } from './store/useStore';
import { THEMES } from './types/themes';
import { ParticleBackground, Scanlines } from './components/ParticleBackground';
import { CodeEditor } from './components/Editor/CodeEditor';
import { FileUploader } from './components/Editor/FileUploader';
import { AutomatonView } from './components/Automaton/AutomatonView';
import { TablesView } from './components/Tables/TablesView';
import { DisambiguationView } from './components/Disambiguation/DisambiguationView';
import { FirstFollowView } from './components/FirstFollow/FirstFollowView';
import { ConsolePanel } from './components/Console/ConsolePanel';
import type { ActiveModule, ActiveView } from './types';
import type { MonacoMarker } from './types/console';

const API = 'http://localhost:8000';

// Vista activa en el panel derecho — incluye "traduccion" para Q'eqchi'
type RightView = ActiveView | 'tokens' | 'traduccion' | 'disambiguation' | 'firstfollow';

const VIEWS: { id: RightView; label: string }[] = [
  { id: 'tokens',          label: 'Tokens'          },
  { id: 'automaton',       label: 'Automata'         },
  { id: 'tables',          label: 'Tablas'           },
  { id: 'firstfollow',     label: 'First/Follow'    },
  { id: 'disambiguation',  label: 'Desambiguacion'  },
  { id: 'traduccion',      label: 'Traduccion'       },
];

// Qué vista mostrar en el panel del editor izquierdo
type EditorPane = 'yalex' | 'yapar' | 'input';

// ── Detección de lenguaje ─────────────────────────────────────
// ORDEN IMPORTANTE: de más específico a más genérico.
// Cada lenguaje busca señales EXCLUSIVAS que no aparecen en otros.
function detectLanguage(content: string, filename: string): ActiveModule {
  const c = content.toLowerCase();
  const f = filename.toLowerCase();

  // ── MessiScript — PRIMERO porque es el más específico ────────
  // Señales exclusivas: "messi", "¡gol!", "agarra", "pelota"
  // Estas palabras NO aparecen en COW ni Q'eqchi'
  const isMessi =
    f.includes('messi') ||
    c.includes('messi') ||
    c.includes('¡gol')  ||
    c.includes('agarra') ||
    c.includes('pelota') ||
    c.includes('jugada');
  if (isMessi) return 'messi';

  // ── OK COMPUTER — SEGUNDO, antes que COW para no colisionar ─────
  // Señales exclusivas: "paranoid", "notok", "okcomputer", "karma police"
  const isOkComputer =
    f.includes('okcomputer') ||
    c.includes('paranoid') && c.includes('notok') ||
    c.includes('okloop') ||
    c.includes('"paranoid"') ||
    c.includes('"notok"') ||
    c.includes('"police"') && c.includes('"karma"') && c.includes('"ghost"');
  if (isOkComputer) return 'okcomputer';

  // ── COW — TERCERO, busca las instrucciones exactas de COW ─────
  // Las instrucciones COW son strings como "moo", "MOO", "OOO"
  // Buscamos entre comillas para no confundir con otras palabras
  // También el nombre del archivo
  const isCow =
    f.includes('cow') ||
    c.includes('"moo"') ||
    c.includes('"moo"') ||
    c.includes('"ooo"') ||
    c.includes('"mmm"') ||
    c.includes('"oom"') ||
    c.includes('"moo"') ||   // loop_end token
    c.includes('zero_mem') ||
    c.includes('print_int') && c.includes('read_int') && c.includes('register');
  if (isCow) return 'cow';

  // ── Q'eqchi' / Maya guatemalteco — TERCERO ────────────────────
  // Señales exclusivas del idioma maya
  const isMaya =
    f.includes('qeqchi') || f.includes('maya') ||
    c.includes("q'eqchi'") || c.includes('qeqchi') ||
    c.includes('aatinob')  || c.includes('winq')   ||
    c.includes('poyanam')  || c.includes('tenamit') ||
    c.includes('verb_exist') || c.includes('noun_child') ||
    c.includes('loc_in')   || c.includes('masc_mark');
  if (isMaya) return 'maya';

  // ── Sin coincidencia → tema genérico (terminal verde) ─────────
  return 'yalex';
}

// ── Extracción de línea:columna desde un mensaje de error ─────
// El core emite mensajes en español: "Error sintactico en linea 1:26"
// o errores léxicos estructurados {line, col}. Esta función cubre
// ambos formatos (y el inglés "line 1:26" por si acaso).
function extractLineCol(text: string): { line?: number; col?: number } {
  if (!text) return {};
  // "linea 12:5" / "línea 12:5" / "line 12:5" / "linea 12"
  const m1 = text.match(/l[ií]ne?a?\s+(\d+)(?::(\d+))?/i);
  if (m1) return { line: Number(m1[1]), col: m1[2] ? Number(m1[2]) : undefined };
  // Patrón genérico "12:5" suelto
  const m2 = text.match(/\b(\d+):(\d+)\b/);
  if (m2) return { line: Number(m2[1]), col: Number(m2[2]) };
  return {};
}

const LANG_LABELS: Partial<Record<ActiveModule, string>> = {
  maya:       "Q'eqchi'",
  messi:      'MessiScript',
  cow:        'COW',
  okcomputer: 'OK COMPUTER',
};

export default function App() {
  const {
    activeModule, setActiveModule,
    activeView,   setActiveView,
    yalexContent, yaparContent, inputText,
    setYalexContent, setYaparContent, setInputText,
    setYalexResult, setYaparResult,
    isLoading, setLoading,
    resetResults,
    globalError, setGlobalError,
    yalexResult, yaparResult,
    addConsoleEntry, addConsoleEntries, clearConsole,
  } = useStore();

  const [rightView,      setRightView]      = useState<RightView>('tokens');
  const [editorPane,     setEditorPane]     = useState<EditorPane>('yalex');
  const [yalexFilename,  setYalexFilename]  = useState('');
  const [yaparFilename,  setYaparFilename]  = useState('');
  const [inputFilename,  setInputFilename]  = useState('');
  const [detectedLang,   setDetectedLang]   = useState<string>('');
  const [apiStatus,      setApiStatus]      = useState<'unknown'|'ok'|'error'>('unknown');
  const [translation,    setTranslation]    = useState<any>(null);
  // Marcadores Monaco por archivo
  const [yalexMarkers,   setYalexMarkers]   = useState<MonacoMarker[]>([]);
  const [yaparMarkers,   setYaparMarkers]   = useState<MonacoMarker[]>([]);
  const [inputMarkers,   setInputMarkers]   = useState<MonacoMarker[]>([]);

  const theme      = THEMES[activeModule] ?? THEMES['yalex'];
  const isTerminal = activeModule === 'yalex' || activeModule === 'yapar';
  const isMaya     = activeModule === 'maya';

  // ── API health check ─────────────────────────────────────
  useEffect(() => {
    fetch(`${API}/`)
      .then(r => r.ok ? setApiStatus('ok') : setApiStatus('error'))
      .catch(() => setApiStatus('error'));
  }, []);

  // ── Aplicar tema ─────────────────────────────────────────
  useEffect(() => {
    const root = document.documentElement;
    root.style.setProperty('--bg',             theme.bg);
    root.style.setProperty('--bg-secondary',   theme.bgSecondary);
    root.style.setProperty('--bg-card',        theme.bgCard);
    root.style.setProperty('--border',         theme.borderColor);
    root.style.setProperty('--text-primary',   theme.textPrimary);
    root.style.setProperty('--text-secondary', theme.textSecondary);
    root.style.setProperty('--text-muted',     theme.textMuted);
    root.style.setProperty('--accent-a',       theme.accentA);
    root.style.setProperty('--accent-b',       theme.accentB);
    document.body.style.background = theme.bg;
  }, [theme]);

  // ── Detectar lenguaje al cargar archivo ──────────────────
  const applyDetection = useCallback((content: string, filename: string) => {
    const lang = detectLanguage(content, filename);
    setActiveModule(lang);
    const label = LANG_LABELS[lang];
    setDetectedLang(label ?? '');
  }, [setActiveModule]);

  const handleLoadYalex = useCallback((content: string, name: string) => {
    resetResults();
    setTranslation(null);
    setYalexMarkers([]);
    setYalexContent(content);
    setYalexFilename(name);
    setEditorPane('yalex');
    setRightView('tokens');
    applyDetection(content, name);
    addConsoleEntry({ level: 'info', source: 'Sistema', message: `Cargado: ${name} (${content.split('\n').length} líneas)` });
  }, [setYalexContent, applyDetection, resetResults, addConsoleEntry]);

  const handleLoadYapar = useCallback((content: string, name: string) => {
    resetResults();
    setTranslation(null);
    setYaparMarkers([]);
    setYaparContent(content);
    setYaparFilename(name);
    setEditorPane('yapar');
    setRightView('automaton');
    applyDetection(content, name);
    addConsoleEntry({ level: 'info', source: 'Sistema', message: `Cargado: ${name} (${content.split('\n').length} líneas)` });
  }, [setYaparContent, applyDetection, resetResults, addConsoleEntry]);

  const handleLoadInput = useCallback((content: string, name: string) => {
    resetResults();
    setTranslation(null);
    setInputMarkers([]);
    setInputText(content.trim());
    setInputFilename(name);
    setEditorPane('input');
    addConsoleEntry({ level: 'info', source: 'Sistema', message: `Cargado input: ${name}` });
  }, [setInputText, resetResults, addConsoleEntry]);

  // ── Reset manual ─────────────────────────────────────────
  const handleReset = useCallback(() => {
    resetResults();
    clearConsole();
    setTranslation(null);
    setDetectedLang('');
    setActiveModule('yalex');
    setYalexFilename('');
    setYaparFilename('');
    setInputFilename('');
    setYalexMarkers([]);
    setYaparMarkers([]);
    setInputMarkers([]);
    setRightView('tokens');
    setEditorPane('yalex');
  }, [resetResults, clearConsole, setActiveModule]);

  // ── Análisis completo ─────────────────────────────────────
  const runAnalysis = async () => {
    // Preflight
    if (!yalexContent.trim() && !yaparContent.trim()) {
      addConsoleEntry({
        level: 'error', source: 'Sistema',
        message: 'No hay archivos cargados.',
        detail: 'Carga al menos un archivo .yalex o .yapar antes de ejecutar.',
      });
      return;
    }
    if (yaparContent.trim() && !yalexContent.trim()) {
      addConsoleEntry({
        level: 'error', source: 'Sistema',
        message: 'Falta el archivo .yalex.',
        detail: 'Para analizar una gramática .yapar necesitas también un archivo .yalex con la definición de tokens.',
      });
      return;
    }
    if (apiStatus === 'error') {
      addConsoleEntry({
        level: 'error', source: 'API',
        message: 'La API está offline.',
        detail: 'Asegúrate de que el servidor Python está corriendo:\n  uvicorn api.main:app --reload',
      });
      return;
    }

    setLoading(true);
    setGlobalError(null);
    setTranslation(null);
    setYalexMarkers([]);
    setYaparMarkers([]);
    setInputMarkers([]);

    // Acumuladores locales de marcadores (los estados se actualizan al final)
    const inputMk: MonacoMarker[] = [];

    addConsoleEntry({ level: 'info', source: 'Sistema', message: `Iniciando análisis — ${new Date().toLocaleTimeString()}` });

    try {
      // ── 1. YALex ──────────────────────────────────────────
      if (yalexContent) {
        addConsoleEntry({ level: 'info', source: 'YALex', message: `Analizando "${yalexFilename || 'archivo.yalex'}"…` });

        const res  = await fetch(`${API}/yalex/analyze`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ yalex_content: yalexContent, input_text: inputText }),
        });
        const data = await res.json();

        if (!res.ok) throw new Error(data.detail || 'Error HTTP en YALex');
        if (data.success === false && data.error) {
          // Error en la especificación .yalex (regla mal formada, etc.)
          const { line, col } = extractLineCol(String(data.error));
          addConsoleEntry({
            level: 'error', source: 'YALex',
            message: 'Error en la especificación léxica (.yalex).',
            detail:  data.error,
            line, col, file: 'yalex',
          });
          if (line) {
            setYalexMarkers([{
              startLineNumber: line, startColumn: col ?? 1,
              endLineNumber:   line, endColumn:   (col ?? 1) + 6,
              message: data.error, severity: 'error',
            }]);
            setEditorPane('yalex');
          }
          throw new Error(`YALex: ${data.error}`);
        }

        setYalexResult(data);

        // Errores léxicos: caracteres del input no reconocidos por el .yalex
        const lexErrors = data.errors ?? [];
        if (lexErrors.length > 0) {
          addConsoleEntries(lexErrors.map((e: any) => ({
            level:   'error' as const,
            source:  'YALex' as const,
            message: e.message || `Carácter no reconocido en línea ${e.line}:${e.col}`,
            line:    e.line, col: e.col, file: 'input' as const,
          })));
          // Marcadores en el editor de input (.txt)
          for (const e of lexErrors) {
            if (e.line) inputMk.push({
              startLineNumber: e.line, startColumn: e.col ?? 1,
              endLineNumber:   e.line, endColumn:   (e.col ?? 1) + 1,
              message: e.message || 'Carácter no reconocido', severity: 'error',
            });
          }
        } else {
          addConsoleEntry({
            level: 'success', source: 'YALex',
            message: `${data.tokens?.length ?? 0} tokens generados — DFA: ${data.dfa?.states?.length ?? 0} estados`,
          });
        }
        setRightView('tokens');
      }

      // ── 2. YAPar ──────────────────────────────────────────
      if (yaparContent) {
        addConsoleEntry({ level: 'info', source: 'YAPar', message: `Analizando "${yaparFilename || 'archivo.yapar'}"…` });

        const res  = await fetch(`${API}/yapar/analyze`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            yapar_content: yaparContent,
            yalex_content: yalexContent,
            input_text:    inputText,
          }),
        });
        const data = await res.json();

        if (!res.ok) throw new Error(data.detail || 'Error HTTP en YAPar');
        if (data.success === false && data.error) {
          // Error en la gramática .yapar (sin %%, no-terminal indefinido, etc.)
          const { line, col } = extractLineCol(String(data.error));
          addConsoleEntry({
            level: 'error', source: 'YAPar',
            message: 'Error en la gramática (.yapar).',
            detail:  data.error,
            line, col, file: 'yapar',
          });
          if (line) {
            setYaparMarkers([{
              startLineNumber: line, startColumn: col ?? 1,
              endLineNumber:   line, endColumn:   (col ?? 1) + 6,
              message: data.error, severity: 'error',
            }]);
            setEditorPane('yapar');
          }
          throw new Error(`YAPar: ${data.error}`);
        }

        setYaparResult(data);

        // ── Conflictos NO resueltos → warnings ───────────────
        // Usamos SLR como fuente principal y de-duplicamos por estado+símbolo
        // para no contar el mismo conflicto dos veces (SLR y LALR).
        const slrConflicts  = data.slr?.conflicts  ?? [];
        const lalrConflicts = data.lalr?.conflicts ?? [];
        const seen = new Set<string>();
        const uniqueConflicts: any[] = [];
        for (const c of [...slrConflicts, ...lalrConflicts]) {
          const key = `${c.state}:${c.symbol}`;
          if (seen.has(key)) continue;
          seen.add(key);
          uniqueConflicts.push(c);
        }

        if (uniqueConflicts.length > 0) {
          addConsoleEntries(uniqueConflicts.map((c: any) => ({
            level:   'warning' as const,
            source:  'YAPar'  as const,
            message: `Conflicto en estado ${c.state} con '${c.symbol}'`,
            detail:  c.description,
          })));
        }

        // ── Conflictos resueltos por precedencia → info ──────
        const slrResolved  = data.slr?.resolvedConflicts  ?? [];
        const resolvedSeen = new Set<string>();
        for (const r of slrResolved) {
          const key = `${r.state}:${r.symbol}`;
          if (resolvedSeen.has(key)) continue;
          resolvedSeen.add(key);
          addConsoleEntry({
            level: 'info', source: 'YAPar',
            message: `Conflicto resuelto en estado ${r.state} con '${r.symbol}' → ${r.resolution}`,
            detail:  r.description,
          });
        }

        // ── Errores de parseo del input (.txt) ───────────────
        const parseErrors = data.errors ?? [];
        if (parseErrors.length > 0) {
          for (const e of parseErrors) {
            const msg = typeof e === 'string' ? e : (e.message ?? JSON.stringify(e));
            const { line, col } = extractLineCol(msg);
            addConsoleEntry({
              level: 'error', source: 'YAPar',
              message: msg, line, col, file: 'input',
            });
            if (line) inputMk.push({
              startLineNumber: line, startColumn: col ?? 1,
              endLineNumber:   line, endColumn:   (col ?? 1) + 1,
              message: msg, severity: 'error',
            });
          }
        }

        if (uniqueConflicts.length === 0 && parseErrors.length === 0) {
          addConsoleEntry({
            level: 'success', source: 'YAPar',
            message: `Gramática válida — ${data.grammar?.productions?.length ?? 0} producciones, ${data.automaton?.states?.length ?? 0} estados`,
          });
        }

        // Si hay errores de parseo, mostrar el input para ver los marcadores
        if (parseErrors.length > 0) setEditorPane('input');
        setRightView('automaton');
      }

      // Aplicar marcadores acumulados del input
      if (inputMk.length > 0) setInputMarkers(inputMk);

      // ── 3. Traducción Q'eqchi' ────────────────────────────
      if (isMaya && inputText) {
        const res  = await fetch(`${API}/natural/translate`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({ text: inputText }),
        });
        const data = await res.json();
        if (res.ok && data.translation) {
          setTranslation(data.translation);
          setRightView('traduccion');
          addConsoleEntry({ level: 'success', source: 'Sistema', message: 'Traducción Q\'eqchi\' completada.' });
        }
      }

    } catch (e: any) {
      const msg = e.message || 'Error de conexión con la API';
      setGlobalError(msg);
      // Solo agregamos a consola si no lo añadimos ya como entry estructurado
      if (!msg.startsWith('YALex:') && !msg.startsWith('YAPar:')) {
        addConsoleEntry({ level: 'error', source: 'API', message: msg });
      }
    } finally {
      setLoading(false);
    }
  };

  // Contenido activo en el editor
  const editorContent = editorPane === 'yalex' ? yalexContent
                      : editorPane === 'yapar' ? yaparContent
                      : inputText;
  const editorLang    = editorPane === 'yalex' ? 'yalex'
                      : editorPane === 'yapar' ? 'yapar'
                      : 'plaintext';
  const hasResult     = !!(yalexResult || yaparResult);

  // Marcadores activos según qué panel está visible
  const activeMarkers = useMemo(() =>
    editorPane === 'yalex' ? yalexMarkers :
    editorPane === 'yapar' ? yaparMarkers : inputMarkers,
  [editorPane, yalexMarkers, yaparMarkers, inputMarkers]);

  // Callback para ir a línea desde consola
  const handleGoToLine = useCallback((file: 'yalex' | 'yapar' | 'input') => {
    // Abrimos el panel correcto — Monaco hace el scroll por los marcadores
    setEditorPane(file);
  }, []);

  const FILE_PANES: { id: EditorPane; label: string; filename: string; color?: string }[] = [
    { id: 'yalex', label: '.yalex', filename: yalexFilename || 'sin cargar' },
    { id: 'yapar', label: '.yapar', filename: yaparFilename || 'sin cargar' },
    { id: 'input', label: '.txt',   filename: inputFilename || 'sin cargar' },
  ];

  return (
    <AnimatePresence mode="wait">
      <motion.div
        key={activeModule}
        initial={{ opacity: 0 }}
        animate={{ opacity: 1 }}
        exit={{ opacity: 0 }}
        transition={{ duration: 0.35 }}
        className="relative flex flex-col h-screen overflow-hidden"
        style={{ background: theme.bg, color: theme.textPrimary, fontFamily: theme.fontFamily }}
      >
        <ParticleBackground config={theme.particles} config2={theme.particles2} />
        {isTerminal && <Scanlines />}
        {theme.headerGradient && (
          <div className="absolute inset-x-0 top-0 h-1 pointer-events-none"
               style={{ background: theme.headerGradient, zIndex: 2 }} />
        )}

        <div className="relative flex flex-col h-full" style={{ zIndex: 2 }}>

          {/* ── HEADER ──────────────────────────────────── */}
          <header className="flex items-center gap-3 px-4 py-2 flex-shrink-0 border-b"
                  style={{ background: theme.bgSecondary, borderColor: theme.borderColor }}>

            {/* Logo */}
            <div className="flex items-center justify-center w-7 h-7 rounded-lg font-bold text-sm"
                 style={{
                   background: isTerminal ? 'transparent' : `linear-gradient(135deg,${theme.accentA},${theme.accentB})`,
                   border:     isTerminal ? `1px solid ${theme.accentA}` : 'none',
                   color:      isTerminal ? theme.accentA : '#fff',
                   boxShadow:  isTerminal ? `0 0 10px ${theme.accentA}44` : 'none',
                 }}>
              {isTerminal ? '>' : 'L'}
            </div>

            <span className="font-bold text-sm tracking-wider"
                  style={{ color: theme.textPrimary, textShadow: isTerminal ? `0 0 8px ${theme.accentA}` : 'none' }}>
              {isTerminal ? '// LexIDE Terminal' : 'LexIDE'}
            </span>

            {/* Badge de lenguaje detectado */}
            <AnimatePresence>
              {detectedLang && (
                <motion.span
                  key={detectedLang}
                  initial={{ opacity: 0, scale: 0.8, x: -8 }}
                  animate={{ opacity: 1, scale: 1,   x: 0  }}
                  exit={{    opacity: 0, scale: 0.8         }}
                  className="px-2 py-0.5 rounded text-xs font-semibold"
                  style={{
                    background: `${theme.accentA}22`,
                    border:     `1px solid ${theme.accentA}66`,
                    color:      theme.accentA,
                  }}>
                  {detectedLang}
                </motion.span>
              )}
            </AnimatePresence>

            <div className="flex-1" />

            {/* Status dot */}
            <span className="flex items-center gap-1.5 text-xs"
                  style={{ color: apiStatus==='ok' ? theme.accentA : apiStatus==='error' ? theme.accentB : theme.textMuted }}>
              <span style={{
                width:'7px', height:'7px', borderRadius:'50%', display:'inline-block',
                background: apiStatus==='ok' ? theme.accentA : apiStatus==='error' ? theme.accentB : theme.textMuted,
                boxShadow: apiStatus==='ok' ? `0 0 6px ${theme.accentA}` : 'none',
              }} />
              {apiStatus==='ok' ? 'API online' : apiStatus==='error' ? 'API offline' : '...'}
            </span>

            {/* Boton Reset */}
            <button
              onClick={handleReset}
              title="Limpiar todo y comenzar de nuevo"
              className="px-3 py-1.5 rounded-lg text-xs transition-all duration-200"
              style={{
                background:  'transparent',
                border:      `1px solid ${theme.borderColor}`,
                color:       theme.textMuted,
                fontFamily:  theme.fontFamily,
                cursor:      'pointer',
              }}
              onMouseEnter={e => {
                (e.currentTarget as HTMLElement).style.borderColor = theme.accentB;
                (e.currentTarget as HTMLElement).style.color       = theme.accentB;
              }}
              onMouseLeave={e => {
                (e.currentTarget as HTMLElement).style.borderColor = theme.borderColor;
                (e.currentTarget as HTMLElement).style.color       = theme.textMuted;
              }}
            >
              {isTerminal ? '[ RESET ]' : 'Reset'}
            </button>

            {/* Boton Ejecutar */}
            <button
              onClick={runAnalysis}
              disabled={isLoading}
              className="flex items-center gap-2 px-4 py-1.5 rounded-lg text-xs font-semibold
                         transition-all duration-200 disabled:opacity-50 disabled:cursor-not-allowed"
              style={{
                background: isTerminal ? 'transparent' : theme.accentA,
                border:     `1px solid ${theme.accentA}`,
                color:      isTerminal ? theme.accentA : '#fff',
                boxShadow:  `0 0 12px ${theme.accentA}44`,
                textShadow: isTerminal ? `0 0 6px ${theme.accentA}` : 'none',
              }}>
              {isLoading
                ? <><div className="w-3 h-3 rounded-full border border-current border-t-transparent animate-spin" />
                    {isTerminal ? 'procesando...' : 'Analizando...'}</>
                : isTerminal ? '[ EJECUTAR ]' : 'Ejecutar'}
            </button>
          </header>


          {/* ── CUERPO ──────────────────────────────────── */}
          <div className="flex flex-1 overflow-hidden">

            {/* ── Panel izquierdo: archivos + editor ────── */}
            <div className="flex flex-col border-r" style={{ width: '42%', borderColor: theme.borderColor }}>

              {/* Barra de archivos */}
              <div className="flex flex-col border-b flex-shrink-0"
                   style={{ background: theme.bgSecondary, borderColor: theme.borderColor }}>

                {FILE_PANES.map(fp => (
                  <div key={fp.id}
                       className="flex items-center gap-2 px-3 py-1.5 border-b text-xs"
                       style={{ borderColor: theme.borderColor, opacity: fp.filename === 'sin cargar' ? 0.5 : 1 }}>

                    {/* Indicador activo */}
                    <span style={{
                      width: '6px', height: '6px', borderRadius: '50%', flexShrink: 0,
                      background: editorPane === fp.id ? theme.accentA : 'transparent',
                      border: `1px solid ${editorPane === fp.id ? theme.accentA : theme.borderColor}`,
                      boxShadow: editorPane === fp.id ? `0 0 5px ${theme.accentA}` : 'none',
                    }} />

                    {/* Nombre archivo — clickeable para ver en editor */}
                    <button
                      onClick={() => setEditorPane(fp.id)}
                      style={{
                        color:      editorPane === fp.id ? theme.textPrimary : theme.textMuted,
                        fontFamily: theme.fontFamily,
                        background: 'none', border: 'none', cursor: 'pointer',
                        fontSize: '11px', textAlign: 'left', flex: 1,
                        textShadow: editorPane === fp.id && isTerminal ? `0 0 4px ${theme.accentA}` : 'none',
                      }}>
                      <span style={{ color: theme.accentA }}>{fp.label}</span>
                      {' '}
                      <span style={{ opacity: 0.7 }}>{fp.filename}</span>
                    </button>

                    {/* Botón cargar */}
                    <FileUploader
                      label={fp.label}
                      theme={theme}
                      small
                      onLoad={fp.id === 'yalex' ? handleLoadYalex
                             : fp.id === 'yapar' ? handleLoadYapar
                             : handleLoadInput}
                    />
                  </div>
                ))}
              </div>

              {/* Monaco Editor */}
              <div className="flex-1 overflow-hidden">
                <CodeEditor
                  language={editorLang}
                  height="100%"
                  theme={theme}
                  value={editorContent}
                  markers={activeMarkers}
                />
              </div>
            </div>

            {/* ── Panel derecho: visualizaciones ────────── */}
            <div className="flex flex-col flex-1 overflow-hidden">

              {/* Tabs de vistas */}
              <div className="flex items-center gap-1 px-3 py-2 border-b flex-shrink-0 flex-wrap"
                   style={{ background: theme.bgSecondary, borderColor: theme.borderColor }}>
                {VIEWS.map(v => {
                  if (v.id === 'traduccion' && !isMaya) return null;
                  const isActive = rightView === v.id;
                  // Badges de error por tab
                  const lexErrCount = v.id === 'tokens' ? (yalexResult?.errors?.length ?? 0) : 0;
                  const parseErrCount = 0;
                  const disambigCount = v.id === 'disambiguation'
                    ? ((yaparResult?.lalr?.resolvedConflicts?.length ?? 0) + (yaparResult?.slr?.resolvedConflicts?.length ?? 0))
                    : 0;
                  const errCount = lexErrCount + parseErrCount;
                  return (
                    <button key={v.id}
                            onClick={() => setRightView(v.id)}
                            className="relative px-3 py-1.5 rounded-lg text-xs font-medium transition-all duration-200"
                            style={{
                              background: isActive ? theme.bgCard : 'transparent',
                              border:     `1px solid ${isActive ? theme.accentA : 'transparent'}`,
                              color:      isActive ? theme.accentA : theme.textMuted,
                              textShadow: isActive && isTerminal ? `0 0 6px ${theme.accentA}88` : 'none',
                            }}>
                      {v.label}
                      {errCount > 0 && (
                        <span style={{
                          position: 'absolute', top: '-4px', right: '-4px',
                          background: '#ef4444', color: '#fff',
                          borderRadius: '50%', width: '14px', height: '14px',
                          fontSize: '9px', fontWeight: 700, lineHeight: '14px',
                          textAlign: 'center', display: 'block',
                        }}>
                          {errCount > 9 ? '9+' : errCount}
                        </span>
                      )}
                      {disambigCount > 0 && errCount === 0 && (
                        <span style={{
                          position: 'absolute', top: '-4px', right: '-4px',
                          background: '#22c55e', color: '#fff',
                          borderRadius: '50%', width: '14px', height: '14px',
                          fontSize: '9px', fontWeight: 700, lineHeight: '14px',
                          textAlign: 'center', display: 'block',
                        }}>
                          {disambigCount > 9 ? '9+' : disambigCount}
                        </span>
                      )}
                    </button>
                  );
                })}
              </div>

              {/* Contenido de la vista */}
              <div className="flex-1 overflow-auto">
                <AnimatePresence mode="wait">
                  <motion.div
                    key={`${rightView}-${activeModule}`}
                    initial={{ opacity: 0, y: 4 }}
                    animate={{ opacity: 1, y: 0 }}
                    exit={{ opacity: 0 }}
                    transition={{ duration: 0.15 }}
                    className="w-full h-full">

                    {/* Tokens */}
                    {rightView === 'tokens' && (
                      <div className="p-4 h-full overflow-auto" style={{ fontFamily: theme.fontFamily }}>
                        {yalexResult ? (
                          <div>
                            {/* Banner de errores léxicos — siempre arriba y visible */}
                            {yalexResult.errors?.length > 0 && (
                              <div className="rounded-xl border-2 p-4 mb-4"
                                   style={{ background: '#1a0505', borderColor: '#ef4444' }}>
                                <p className="text-xs font-bold mb-2 flex items-center gap-2"
                                   style={{ color: '#ef4444' }}>
                                  <span>✗</span>
                                  {yalexResult.errors.length} error{yalexResult.errors.length > 1 ? 'es' : ''} léxico{yalexResult.errors.length > 1 ? 's' : ''} — caracteres no reconocidos por el .yalex
                                </p>
                                <div className="flex flex-col gap-1.5">
                                  {yalexResult.errors.map((e: any, i: number) => (
                                    <div key={i} className="rounded px-3 py-2 text-xs font-mono flex items-start gap-3"
                                         style={{ background: '#ff000018', color: '#fca5a5', borderLeft: '3px solid #ef4444' }}>
                                      <span style={{ color: '#ef444499', fontWeight: 700, flexShrink: 0 }}>
                                        {e.line}:{e.col}
                                      </span>
                                      <span>{e.message}</span>
                                    </div>
                                  ))}
                                </div>
                              </div>
                            )}
                            <p className="text-xs mb-3" style={{ color: theme.textMuted }}>
                              {yalexResult.tokens.length} tokens encontrados
                              {yalexResult.dfa?.states && ` — DFA: ${yalexResult.dfa.states.length} estados`}
                            </p>
                            <div className="flex flex-wrap gap-1.5">
                              {yalexResult.tokens.map((t: any, i: number) => (
                                <span key={i}
                                      className="px-2 py-1 rounded text-xs"
                                      style={{
                                        background: t.type === 'EOF'
                                          ? `${theme.textMuted}22`
                                          : `${theme.accentA}18`,
                                        border: `1px solid ${t.type === 'EOF'
                                          ? theme.borderColor
                                          : theme.accentA}44`,
                                        color: t.type === 'EOF'
                                          ? theme.textMuted
                                          : theme.textPrimary,
                                      }}>
                                  <span style={{ color: theme.accentA, fontSize: '10px' }}>{t.type}</span>
                                  {t.lexeme && <span style={{ color: theme.textSecondary }}> "{t.lexeme}"</span>}
                                  <span style={{ color: theme.textMuted, fontSize: '10px' }}> {t.line}:{t.col}</span>
                                </span>
                              ))}
                            </div>
                          </div>
                        ) : (
                          <p className="text-xs" style={{ color: theme.textMuted }}>
                            {isTerminal ? '// carga un .yalex y ejecuta' : 'Carga un archivo .yalex y ejecuta'}
                          </p>
                        )}
                      </div>
                    )}

                    {rightView === 'automaton'      && <AutomatonView       theme={theme} />}
                    {rightView === 'tables'         && <TablesView          theme={theme} />}
                    {rightView === 'firstfollow'    && <FirstFollowView     theme={theme} />}
                    {rightView === 'disambiguation'  && <DisambiguationView  theme={theme} />}

                    {/* Traduccion Q'eqchi' */}
                    {rightView === 'traduccion' && (
                      <div className="p-5 h-full overflow-auto" style={{ fontFamily: theme.fontFamily }}>
                        {translation ? (
                          <div className="flex flex-col gap-5">

                            {/* Bloque original / traducción */}
                            <div className="rounded-xl p-4"
                                 style={{ background: `${theme.accentA}12`, border: `1px solid ${theme.accentA}33` }}>
                              <p className="text-xs mb-1" style={{ color: theme.textMuted }}>Q'eqchi'</p>
                              <p className="text-sm font-medium" style={{ color: theme.textPrimary }}>
                                {translation.original}
                              </p>
                            </div>

                            <div className="rounded-xl p-4"
                                 style={{ background: `${theme.accentB}12`, border: `1px solid ${theme.accentB}33` }}>
                              <p className="text-xs mb-1" style={{ color: theme.textMuted }}>Español</p>
                              <p className="text-sm font-medium" style={{ color: theme.textPrimary }}>
                                {translation.spanish}
                              </p>
                            </div>

                            {/* Tabla palabra por palabra */}
                            <div>
                              <p className="text-xs mb-2" style={{ color: theme.textMuted }}>
                                Traduccion palabra por palabra
                              </p>
                              <div className="rounded-xl overflow-hidden"
                                   style={{ border: `1px solid ${theme.borderColor}` }}>
                                {translation.word_map?.map((w: any, i: number) => (
                                  <div key={i}
                                       className="flex items-center gap-3 px-3 py-1.5 text-xs border-b"
                                       style={{
                                         borderColor: theme.borderColor,
                                         background: i % 2 === 0 ? theme.bgCard : 'transparent',
                                       }}>
                                    <span style={{ color: theme.accentA, minWidth: '120px', fontWeight: 600 }}>
                                      {w.qeqchi}
                                    </span>
                                    <span style={{ color: theme.textMuted }}>→</span>
                                    <span style={{ color: theme.textSecondary }}>{w.spanish}</span>
                                  </div>
                                ))}
                              </div>
                            </div>

                            {/* Nota gramatical */}
                            <div className="rounded-xl p-3 text-xs"
                                 style={{ background: theme.bgCard, border: `1px solid ${theme.borderColor}`,
                                          color: theme.textMuted }}>
                              {translation.notes}
                            </div>

                          </div>
                        ) : (
                          <div className="flex flex-col items-center justify-center h-full gap-3">
                            <p className="text-sm" style={{ color: theme.textMuted }}>
                              Carga un archivo Q'eqchi' y ejecuta para ver la traduccion
                            </p>
                          </div>
                        )}
                      </div>
                    )}

                  </motion.div>
                </AnimatePresence>
              </div>
            </div>
          </div>

          {/* ── CONSOLA ─────────────────────────────────── */}
          <ConsolePanel theme={theme} onGoToLine={handleGoToLine} />

          {/* ── FOOTER ──────────────────────────────────── */}
          <footer className="flex items-center gap-3 px-4 py-1.5 border-t text-xs flex-shrink-0"
                  style={{ background: theme.bgSecondary, borderColor: theme.borderColor,
                           color: theme.textMuted, fontFamily: theme.fontFamily }}>

            {hasResult ? (
              <span style={{ color: theme.accentA, textShadow: isTerminal ? `0 0 4px ${theme.accentA}` : 'none' }}>
                {isTerminal ? '[ OK ]' : 'OK'}
              </span>
            ) : (
              <span>{isTerminal ? '[ LISTO ]' : 'Listo'}</span>
            )}

            {yaparResult && (
              <>
                <span style={{ color: theme.borderColor }}>|</span>
                <span>{yaparResult.grammar?.productions?.length ?? 0} producciones</span>
                <span style={{ color: theme.borderColor }}>|</span>
                <span>{yaparResult.automaton?.states?.length ?? 0} estados</span>
                <span style={{ color: theme.borderColor }}>|</span>
                <span style={{ color: (yaparResult.slr?.conflicts?.length||0)>0 ? theme.accentB : theme.accentA }}>
                  {yaparResult.slr?.conflicts?.length || 0} conflictos
                </span>
              </>
            )}

            {yalexResult && (
              <>
                <span style={{ color: theme.borderColor }}>|</span>
                <span>{yalexResult.tokens?.length ?? 0} tokens</span>
                <span style={{ color: theme.borderColor }}>|</span>
                <span>{yalexResult.dfa?.states?.length ?? 0} estados DFA</span>
              </>
            )}

            <div className="flex-1" />

            {/* Indicador de errores en footer */}
            {(yalexResult?.errors?.length || yaparResult?.errors?.length || (!yaparResult?.success && yaparResult)) ? (
              <span className="px-2 py-0.5 rounded text-xs flex items-center gap-1"
                    style={{ color: '#ef4444', background: '#ef444422', border: '1px solid #ef444444' }}>
                ✗ {(yalexResult?.errors?.length ?? 0) + (yaparResult?.errors?.length ?? 0)} errores
              </span>
            ) : hasResult ? (
              <span className="px-2 py-0.5 rounded text-xs flex items-center gap-1"
                    style={{ color: theme.accentA, background: `${theme.accentA}18` }}>
                ✓ OK
              </span>
            ) : null}

            <span style={{ color: theme.textMuted }}>LexIDE v1.0</span>
          </footer>
        </div>
      </motion.div>
    </AnimatePresence>
  );
}
