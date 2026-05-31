// ============================================================
//  App.tsx — Layout principal del IDE
//
//  Estructura:
//  ┌─────────────────────────────────────────────────────┐
//  │  Header: logo + módulos + run button                │
//  ├──────────────────┬──────────────────────────────────┤
//  │  Panel izquierdo │  Panel derecho                   │
//  │  - Editor código │  - Autómata / Tablas / Árbol     │
//  │  - Input text    │  - Resultados                    │
//  ├──────────────────┴──────────────────────────────────┤
//  │  Error panel                                        │
//  └─────────────────────────────────────────────────────┘
// ============================================================
import { useState } from 'react';
import { motion, AnimatePresence } from 'framer-motion';
import { useStore } from './store/useStore';
import { CodeEditor } from './components/Editor/CodeEditor';
import { AutomatonView } from './components/Automaton/AutomatonView';
import { TablesView } from './components/Tables/TablesView';
import { TreeView } from './components/SyntaxTree/TreeView';
import { ParallelView } from './components/ParallelView/ParallelView';
import type { ActiveModule, ActiveView } from './types';

const API = 'http://localhost:8000';

// ── Módulos disponibles ───────────────────────────────────────
const MODULES: { id: ActiveModule; label: string; icon: string; color: string }[] = [
  { id: 'yalex',  label: 'YALex',       icon: '🔤', color: 'text-purple-400' },
  { id: 'yapar',  label: 'YAPar',       icon: '🌳', color: 'text-teal-400'   },
  { id: 'maya',   label: "Q'eqchi'",    icon: '🌿', color: 'text-green-400'  },
  { id: 'cow',    label: 'COW',         icon: '🐄', color: 'text-orange-400' },
  { id: 'messi',  label: 'MessiScript', icon: '⚽', color: 'text-yellow-400' },
];

// ── Vistas del panel derecho ──────────────────────────────────
const VIEWS: { id: ActiveView; label: string; icon: string }[] = [
  { id: 'automaton', label: 'Autómata',  icon: '🔷' },
  { id: 'tables',    label: 'Tablas',    icon: '📊' },
  { id: 'tree',      label: 'Árbol',     icon: '🌳' },
  { id: 'parallel',  label: 'Paralelo',  icon: '⚡' },
];

export default function App() {
  const {
    activeModule, setActiveModule,
    activeView,   setActiveView,
    yalexContent, yaparContent, inputText,
    setYalexResult, setYaparResult,
    isLoading, setLoading,
    globalError, setGlobalError,
    yalexResult, yaparResult,
  } = useStore();

  const [showInput, setShowInput] = useState(true);

  // ── Ejecutar análisis ─────────────────────────────────────
  const runAnalysis = async () => {
    setLoading(true);
    setGlobalError(null);

    try {
      if (activeModule === 'yalex') {
        const res = await fetch(`${API}/yalex/analyze`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            yalex_content: yalexContent,
            input_text:    inputText,
          }),
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.detail || 'Error en YALex');
        setYalexResult(data);
        setActiveView('tables'); // Mostrar tokens directamente

      } else if (activeModule === 'yapar') {
        const res = await fetch(`${API}/yapar/analyze`, {
          method: 'POST',
          headers: { 'Content-Type': 'application/json' },
          body: JSON.stringify({
            yapar_content: yaparContent,
            yalex_content: yalexContent,
            input_text:    inputText,
          }),
        });
        const data = await res.json();
        if (!res.ok) throw new Error(data.detail || 'Error en YAPar');
        setYaparResult(data);
        setActiveView('automaton');
      }
    } catch (e: any) {
      setGlobalError(e.message || 'Error de conexión con el API');
    } finally {
      setLoading(false);
    }
  };

  const currentResult = activeModule === 'yalex' ? yalexResult : yaparResult;
  const hasError = currentResult && !currentResult.success;

  return (
    <div className="flex flex-col h-screen bg-bg text-text-primary overflow-hidden">

      {/* ── Header ─────────────────────────────────────────── */}
      <header className="flex items-center gap-4 px-4 py-2 bg-bg-secondary border-b border-border flex-shrink-0">
        {/* Logo */}
        <div className="flex items-center gap-2 mr-2">
          <div className="w-7 h-7 rounded-lg bg-gradient-to-br from-accent-purple to-accent-teal flex items-center justify-center text-sm">
            ⚙
          </div>
          <span className="font-bold text-sm hidden sm:block text-text-primary">LexIDE</span>
        </div>

        {/* Módulos */}
        <div className="flex gap-1">
          {MODULES.map(m => (
            <button
              key={m.id}
              onClick={() => setActiveModule(m.id)}
              className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium transition-all
                ${activeModule === m.id
                  ? 'bg-bg-card border border-border text-text-primary'
                  : 'text-text-muted hover:text-text-secondary hover:bg-bg-card/50'
                }`}
            >
              <span>{m.icon}</span>
              <span className={`hidden md:block ${activeModule === m.id ? m.color : ''}`}>{m.label}</span>
            </button>
          ))}
        </div>

        <div className="flex-1" />

        {/* Botón Run */}
        <button
          onClick={runAnalysis}
          disabled={isLoading}
          className="flex items-center gap-2 px-4 py-1.5 bg-accent-purple hover:bg-purple-600
                     disabled:opacity-50 disabled:cursor-not-allowed
                     text-white rounded-lg text-xs font-semibold transition-all
                     shadow-lg shadow-purple-900/30"
        >
          {isLoading ? (
            <>
              <div className="w-3 h-3 border border-white/40 border-t-white rounded-full animate-spin" />
              Analizando...
            </>
          ) : (
            <> ▶ Ejecutar </>
          )}
        </button>
      </header>

      {/* ── Cuerpo principal ────────────────────────────────── */}
      <div className="flex flex-1 overflow-hidden">

        {/* Panel izquierdo — Editor */}
        <div className="flex flex-col w-[45%] border-r border-border overflow-hidden">

          {/* Toolbar del editor */}
          <div className="flex items-center gap-2 px-3 py-2 bg-bg-secondary border-b border-border flex-shrink-0">
            <span className="text-xs font-mono text-text-muted">
              {activeModule === 'yalex' ? 'archivo.yalex' :
               activeModule === 'yapar' ? 'archivo.yapar' :
               `${activeModule}.lang`}
            </span>
            <div className="flex-1" />
            <button
              onClick={() => setShowInput(!showInput)}
              className="text-xs text-text-secondary hover:text-text-primary transition-colors"
            >
              {showInput ? 'Ocultar input' : 'Mostrar input'}
            </button>
          </div>

          {/* Editor principal */}
          <div className={`${showInput ? 'flex-[3]' : 'flex-1'} overflow-hidden`}>
            <CodeEditor
              language={activeModule === 'yalex' ? 'yalex' :
                        activeModule === 'yapar' ? 'yapar' : 'plaintext'}
              height="100%"
            />
          </div>

          {/* Panel de input */}
          <AnimatePresence>
            {showInput && (
              <motion.div
                initial={{ height: 0, opacity: 0 }}
                animate={{ height: 'auto', opacity: 1 }}
                exit={{ height: 0, opacity: 0 }}
                transition={{ duration: 0.2 }}
                className="flex flex-col border-t border-border overflow-hidden"
              >
                <div className="flex items-center gap-2 px-3 py-1.5 bg-bg-secondary border-b border-border">
                  <span className="text-xs text-text-muted">Input de prueba</span>
                </div>
                <div className="flex-1 min-h-[80px] max-h-[120px]">
                  <CodeEditor language="plaintext" height="100px" />
                </div>
              </motion.div>
            )}
          </AnimatePresence>
        </div>

        {/* Panel derecho — Visualizaciones */}
        <div className="flex flex-col flex-1 overflow-hidden">

          {/* Tabs de vista */}
          <div className="flex items-center gap-1 px-3 py-2 bg-bg-secondary border-b border-border flex-shrink-0">
            {VIEWS.map(v => (
              <button
                key={v.id}
                onClick={() => setActiveView(v.id)}
                className={`flex items-center gap-1.5 px-3 py-1.5 rounded-lg text-xs font-medium transition-all
                  ${activeView === v.id
                    ? 'bg-bg-card border border-border text-text-primary'
                    : 'text-text-muted hover:text-text-secondary'
                  }`}
              >
                <span>{v.icon}</span>
                <span className="hidden sm:block">{v.label}</span>
              </button>
            ))}
          </div>

          {/* Contenido de la vista activa */}
          <div className="flex-1 overflow-hidden">
            <AnimatePresence mode="wait">
              <motion.div
                key={activeView}
                initial={{ opacity: 0, y: 4 }}
                animate={{ opacity: 1, y: 0 }}
                exit={{ opacity: 0 }}
                transition={{ duration: 0.15 }}
                className="w-full h-full"
              >
                {activeView === 'automaton' && <AutomatonView />}
                {activeView === 'tables'    && <TablesView />}
                {activeView === 'tree'      && <TreeView />}
                {activeView === 'parallel'  && <ParallelView />}
              </motion.div>
            </AnimatePresence>
          </div>
        </div>
      </div>

      {/* ── Barra de estado inferior ─────────────────────────── */}
      <footer className="flex items-center gap-4 px-4 py-1.5 bg-bg-secondary border-t border-border text-xs flex-shrink-0">
        {/* Estado del análisis */}
        {currentResult ? (
          <div className={`flex items-center gap-1.5 ${hasError ? 'text-accent-red' : 'text-accent-green'}`}>
            <div className={`w-2 h-2 rounded-full ${hasError ? 'bg-accent-red' : 'bg-accent-green'}`} />
            {hasError ? 'Error en el análisis' : 'Análisis exitoso'}
          </div>
        ) : (
          <div className="flex items-center gap-1.5 text-text-muted">
            <div className="w-2 h-2 rounded-full bg-text-muted" />
            Listo
          </div>
        )}

        {yaparResult && (
          <>
            <span className="text-text-muted">|</span>
            <span className="text-text-secondary">
              {yaparResult.grammar?.productions?.length} producciones
            </span>
            <span className="text-text-muted">|</span>
            <span className="text-text-secondary">
              {yaparResult.automaton?.states?.length} estados LR(0)
            </span>
            <span className="text-text-muted">|</span>
            <span className={yaparResult.slr?.conflicts?.length > 0 ? 'text-accent-orange' : 'text-accent-green'}>
              {yaparResult.slr?.conflicts?.length || 0} conflictos SLR
            </span>
          </>
        )}

        {yalexResult && (
          <>
            <span className="text-text-muted">|</span>
            <span className="text-text-secondary">
              {yalexResult.tokens?.length} tokens
            </span>
            {yalexResult.dfa && (
              <>
                <span className="text-text-muted">|</span>
                <span className="text-text-secondary">
                  {yalexResult.dfa.states?.length} estados DFA
                </span>
              </>
            )}
          </>
        )}

        <div className="flex-1" />

        {/* Error global */}
        {globalError && (
          <span className="text-accent-red bg-red-950/30 px-2 py-0.5 rounded">
            ⚠ {globalError}
          </span>
        )}

        <span className="text-text-muted">LexIDE v1.0</span>
      </footer>
    </div>
  );
}
