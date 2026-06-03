// ============================================================
//  FileUploader.tsx — Carga de archivos de texto
// ============================================================
import { useRef } from 'react';
import type { ModuleTheme } from '../../types/themes';

interface Props {
  label:  string;
  onLoad: (content: string, filename: string) => void;
  theme:  ModuleTheme;
  small?: boolean;
}

export function FileUploader({ label, onLoad, theme, small }: Props) {
  const inputRef = useRef<HTMLInputElement>(null);

  const handleChange = (e: React.ChangeEvent<HTMLInputElement>) => {
    const file = e.target.files?.[0];
    if (!file) return;

    const reader = new FileReader();
    reader.onload = (ev) => {
      const content = ev.target?.result as string;
      if (content !== undefined) {
        onLoad(content, file.name);
      }
      // Reset DESPUÉS de que terminó la lectura
      if (inputRef.current) inputRef.current.value = '';
    };
    reader.onerror = () => {
      alert('No se pudo leer el archivo');
      if (inputRef.current) inputRef.current.value = '';
    };
    reader.readAsText(file, 'utf-8');
  };

  const pad = small ? '3px 10px' : '5px 12px';

  return (
    <label
      style={{
        display:      'inline-flex',
        alignItems:   'center',
        gap:          '5px',
        padding:      pad,
        fontSize:     '11px',
        fontFamily:   theme.fontFamily,
        background:   'transparent',
        border:       `1px solid ${theme.borderColor}`,
        borderRadius: '6px',
        color:        theme.textSecondary,
        cursor:       'pointer',
        userSelect:   'none',
        transition:   'border-color 0.15s, color 0.15s',
        whiteSpace:   'nowrap',
      }}
      onMouseEnter={e => {
        const el = e.currentTarget as HTMLElement;
        el.style.borderColor = theme.accentA;
        el.style.color       = theme.textPrimary;
      }}
      onMouseLeave={e => {
        const el = e.currentTarget as HTMLElement;
        el.style.borderColor = theme.borderColor;
        el.style.color       = theme.textSecondary;
      }}
    >
      {/* Input oculto — el click se delega al label */}
      <input
        ref={inputRef}
        type="file"
        onChange={handleChange}
        style={{ display: 'none' }}
      />
      Cargar {label}
    </label>
  );
}
