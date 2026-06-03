// ============================================================
//  CodeEditor.tsx — Monaco Editor con tema dinámico y marcadores
// ============================================================
import Editor, { type Monaco } from '@monaco-editor/react';
import { useEffect, useRef } from 'react';
import { useStore } from '../../store/useStore';
import type { ModuleTheme } from '../../types/themes';
import type { MonacoMarker } from '../../types/console';

interface Props {
  language: 'yalex' | 'yapar' | 'plaintext';
  height?:  string;
  theme:    ModuleTheme;
  value?:   string;
  markers?: MonacoMarker[];
}

export function CodeEditor({ language, height = '100%', theme, value: externalValue, markers = [] }: Props) {
  const {
    yalexContent, yaparContent, inputText,
    setYalexContent, setYaparContent, setInputText,
    activeModule,
  } = useStore();

  const monacoRef = useRef<Monaco | null>(null);
  const editorRef = useRef<any>(null);

  const isTerminal = activeModule === 'yalex' || activeModule === 'yapar';

  const value = externalValue !== undefined
    ? externalValue
    : language === 'plaintext'
      ? inputText
      : activeModule === 'yalex' ? yalexContent : yaparContent;

  const onChange = (val?: string) => {
    if (!val) return;
    if (language === 'plaintext') setInputText(val);
    else if (activeModule === 'yalex') setYalexContent(val);
    else setYaparContent(val);
  };

  // Aplicar marcadores cuando cambian
  useEffect(() => {
    if (!monacoRef.current || !editorRef.current) return;
    const model = editorRef.current.getModel();
    if (!model) return;

    const monaco = monacoRef.current;
    const monacoMarkers = markers.map(m => ({
      startLineNumber: m.startLineNumber,
      startColumn:     m.startColumn,
      endLineNumber:   m.endLineNumber,
      endColumn:       m.endColumn,
      message:         m.message,
      severity:
        m.severity === 'error'   ? monaco.MarkerSeverity.Error   :
        m.severity === 'warning' ? monaco.MarkerSeverity.Warning :
                                   monaco.MarkerSeverity.Info,
    }));

    monaco.editor.setModelMarkers(model, 'lexide', monacoMarkers);
  }, [markers]);

  return (
    <Editor
      height={height}
      language="plaintext"
      value={value}
      onChange={onChange}
      theme="lexide-theme"
      beforeMount={(monaco) => {
        monacoRef.current = monaco;
        monaco.editor.defineTheme('lexide-theme', {
          base: 'vs-dark',
          inherit: true,
          rules: [
            { token: 'comment', foreground: theme.textMuted.replace('#', '') },
          ],
          colors: {
            'editor.background':               theme.bgCard,
            'editor.foreground':               theme.textPrimary,
            'editor.lineHighlightBackground':  theme.bgSecondary,
            'editorLineNumber.foreground':     theme.textMuted.replace('#', ''),
            'editorLineNumber.activeForeground': theme.textSecondary.replace('#', ''),
            'editor.selectionBackground':      `${theme.accentA}33`,
            'editorCursor.foreground':         theme.accentA,
            'editor.inactiveSelectionBackground': theme.bgSecondary,
            'editorWidget.background':         theme.bgCard,
            'input.background':                theme.bgSecondary,
            // Subrayado de error rojo
            'editorError.foreground':   '#f87171',
            'editorError.border':       '#f87171',
            'editorWarning.foreground': '#fbbf24',
            'editorWarning.border':     '#fbbf24',
          },
        });
      }}
      onMount={(editor, monaco) => {
        editorRef.current  = editor;
        monacoRef.current  = monaco;
      }}
      options={{
        fontSize:              14,
        fontFamily:            theme.fontFamily,
        fontLigatures:         true,
        lineNumbers:           'on',
        minimap:               { enabled: false },
        scrollBeyondLastLine:  false,
        wordWrap:              'on',
        tabSize:               2,
        renderLineHighlight:   'line',
        smoothScrolling:       true,
        cursorSmoothCaretAnimation: 'on',
        padding:               { top: 12, bottom: 12 },
        cursorBlinking: isTerminal ? 'blink' : 'smooth',
        cursorStyle:    isTerminal ? 'block' : 'line',
      }}
    />
  );
}
