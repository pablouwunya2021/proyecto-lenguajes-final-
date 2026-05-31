// ============================================================
//  CodeEditor.tsx — Editor Monaco con syntax highlighting
// ============================================================
import Editor from '@monaco-editor/react';
import { useStore } from '../../store/useStore';

// Tema personalizado oscuro para el IDE
const DARK_THEME = {
  base: 'vs-dark' as const,
  inherit: true,
  rules: [
    { token: 'comment',  foreground: '6e7681' },
    { token: 'keyword',  foreground: 'ff7b72' },
    { token: 'string',   foreground: 'a5d6ff' },
    { token: 'number',   foreground: 'ffa657' },
    { token: 'operator', foreground: '79c0ff' },
  ],
  colors: {
    'editor.background':          '#161b22',
    'editor.foreground':          '#e6edf3',
    'editor.lineHighlightBackground': '#1c2128',
    'editorLineNumber.foreground':    '#484f58',
    'editorLineNumber.activeForeground': '#8b949e',
    'editor.selectionBackground': '#264f7840',
    'editorCursor.foreground':    '#7c3aed',
    'editor.inactiveSelectionBackground': '#1c2128',
  },
};

interface Props {
  language: 'yalex' | 'yapar' | 'plaintext';
  height?: string;
}

export function CodeEditor({ language, height = '100%' }: Props) {
  const {
    yalexContent, yaparContent, inputText,
    setYalexContent, setYaparContent, setInputText,
    activeModule,
  } = useStore();

  const value = language === 'plaintext'
    ? inputText
    : activeModule === 'yalex' ? yalexContent : yaparContent;

  const onChange = (val: string | undefined) => {
    if (!val) return;
    if (language === 'plaintext') setInputText(val);
    else if (activeModule === 'yalex') setYalexContent(val);
    else setYaparContent(val);
  };

  const monacoLang = language === 'plaintext' ? 'plaintext' : 'plaintext';

  return (
    <Editor
      height={height}
      language={monacoLang}
      value={value}
      onChange={onChange}
      theme="lexide-dark"
      beforeMount={(monaco) => {
        monaco.editor.defineTheme('lexide-dark', DARK_THEME);
      }}
      options={{
        fontSize: 14,
        fontFamily: "'JetBrains Mono', 'Fira Code', monospace",
        fontLigatures: true,
        lineNumbers: 'on',
        minimap: { enabled: false },
        scrollBeyondLastLine: false,
        wordWrap: 'on',
        tabSize: 2,
        renderLineHighlight: 'line',
        smoothScrolling: true,
        cursorSmoothCaretAnimation: 'on',
        padding: { top: 16, bottom: 16 },
      }}
    />
  );
}
