// ============================================================
//  console.ts — Tipos para el sistema de consola de errores
// ============================================================

export type LogLevel  = 'error' | 'warning' | 'info' | 'success';
export type LogSource = 'YALex' | 'YAPar' | 'Sistema' | 'API';

export interface ConsoleEntry {
  id:        string;
  level:     LogLevel;
  source:    LogSource;
  message:   string;
  detail?:   string;      // descripción técnica adicional
  line?:     number;
  col?:      number;
  file?:     'yalex' | 'yapar' | 'input';
  timestamp: number;
}

export interface MonacoMarker {
  startLineNumber: number;
  startColumn:     number;
  endLineNumber:   number;
  endColumn:       number;
  message:         string;
  severity:        'error' | 'warning' | 'info';
}
