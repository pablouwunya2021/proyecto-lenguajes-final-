// ============================================================
//  themes.ts — Configuración de temas por módulo
// ============================================================
import type { ActiveModule } from './index';

export interface ParticleConfig {
  count:    number;
  color:    string;
  size:     [number, number];   // [min, max]
  speed:    [number, number];
  opacity:  [number, number];
  shape:    'circle' | 'square' | 'char';
  char?:    string;             // si shape === 'char'
}

export interface ModuleTheme {
  id:           ActiveModule;
  label:        string;         // sin emojis
  bg:           string;         // color de fondo principal
  bgSecondary:  string;
  bgCard:       string;
  borderColor:  string;
  textPrimary:  string;
  textSecondary:string;
  textMuted:    string;
  accentA:      string;         // color acento principal
  accentB:      string;         // color acento secundario
  fontFamily:   string;
  particles:    ParticleConfig | null;
  particles2?:  ParticleConfig | null;  // capa extra de partículas (ej: segundo color)
  // Efecto extra: scanlines para terminal
  scanlines?:   boolean;
  // Gradiente de fondo para algunas secciones
  headerGradient?: string;
}

export const THEMES: Record<ActiveModule, ModuleTheme> = {

  // ── YALex: terminal verde sobre negro ─────────────────────
  yalex: {
    id: 'yalex',
    label: 'YALex',
    bg:            '#020904',
    bgSecondary:   '#040c05',
    bgCard:        '#071008',
    borderColor:   '#0f3d12',
    textPrimary:   '#00ff41',
    textSecondary: '#00cc33',
    textMuted:     '#005c17',
    accentA:       '#00ff41',
    accentB:       '#ff3131',
    fontFamily:    "'JetBrains Mono', 'Courier New', monospace",
    scanlines:     true,
    particles: {
      count:  40,
      color:  '#00ff41',
      size:   [1, 3],
      speed:  [0.2, 0.8],
      opacity:[0.1, 0.4],
      shape:  'char',
      char:   '01',
    },
  },

  // ── YAPar: terminal rojo/ámbar sobre negro ─────────────────
  yapar: {
    id: 'yapar',
    label: 'YAPar',
    bg:            '#060200',
    bgSecondary:   '#0a0400',
    bgCard:        '#100600',
    borderColor:   '#3d1500',
    textPrimary:   '#ff6b1a',
    textSecondary: '#cc5500',
    textMuted:     '#5c2500',
    accentA:       '#ff6b1a',
    accentB:       '#ff3131',
    fontFamily:    "'JetBrains Mono', 'Courier New', monospace",
    scanlines:     true,
    particles: {
      count:  35,
      color:  '#ff6b1a',
      size:   [1, 2],
      speed:  [0.3, 0.9],
      opacity:[0.08, 0.3],
      shape:  'char',
      char:   '→|·',
    },
  },

  // ── Maya Q'eqchi': azul y blanco (bandera de Guatemala) ───
  maya: {
    id: 'maya',
    label: "Q'eqchi'",
    bg:            '#001a3d',
    bgSecondary:   '#002050',
    bgCard:        '#002766',
    borderColor:   '#1a4d8f',
    textPrimary:   '#e8f4ff',
    textSecondary: '#90c4f0',
    textMuted:     '#4a80b0',
    accentA:       '#4fc3f7',
    accentB:       '#ffffff',
    fontFamily:    "'Inter', 'Georgia', serif",
    headerGradient:'linear-gradient(135deg, #003a8c 0%, #4a9eff 50%, #003a8c 100%)',
    particles: {
      count:  50,
      color:  '#ffffff',
      size:   [2, 5],
      speed:  [0.1, 0.5],
      opacity:[0.1, 0.5],
      shape:  'circle',
    },
  },

  // ── MessiScript: azul y rojo difuminado (Barcelona) ────────
  messi: {
    id: 'messi',
    label: 'MessiScript',
    bg:            '#001133',
    bgSecondary:   '#00194d',
    bgCard:        '#001f5c',
    borderColor:   '#1a3a7a',
    textPrimary:   '#f0f4ff',
    textSecondary: '#8aa8e0',
    textMuted:     '#3a5080',
    accentA:       '#ffd700',
    accentB:       '#cc0000',
    fontFamily:    "'Inter', system-ui, sans-serif",
    headerGradient:'linear-gradient(135deg, #004d98 0%, #6a0dad 50%, #a50044 100%)',
    particles: {
      count:  60,
      color:  '#ffd700',
      size:   [2, 6],
      speed:  [0.2, 0.7],
      opacity:[0.1, 0.5],
      shape:  'circle',
    },
  },

  // ── OK COMPUTER: blanco, azul y azul claro con binario ──────
  okcomputer: {
    id: 'okcomputer',
    label: 'OK COMPUTER',
    bg:            '#ffffff',
    bgSecondary:   '#f0f6ff',
    bgCard:        '#e8f0fe',
    borderColor:   '#bbdefb',
    textPrimary:   '#0d1b4b',
    textSecondary: '#1565c0',
    textMuted:     '#90caf9',
    accentA:       '#1565c0',   // azul
    accentB:       '#42a5f5',   // azul claro
    fontFamily:    "'JetBrains Mono', 'Courier New', monospace",
    headerGradient:'linear-gradient(135deg, #1565c0 0%, #42a5f5 50%, #1565c0 100%)',
    particles: {
      count:   35,
      color:   '#1565c0',
      size:    [4, 9],
      speed:   [0.08, 0.4],
      opacity: [0.07, 0.18],
      shape:   'char',
      char:    '01',
    },
    particles2: {
      count:   30,
      color:   '#42a5f5',
      size:    [3, 7],
      speed:   [0.1, 0.5],
      opacity: [0.05, 0.14],
      shape:   'char',
      char:    '01',
    },
  },

  // ── COW: blanco con partículas negras ───────────────────────
  cow: {
    id: 'cow',
    label: 'COW',
    bg:            '#f5f5f0',
    bgSecondary:   '#ece9e0',
    bgCard:        '#ffffff',
    borderColor:   '#d0ccbe',
    textPrimary:   '#1a1a1a',
    textSecondary: '#4a4a4a',
    textMuted:     '#9a9a9a',
    accentA:       '#1a1a1a',
    accentB:       '#555555',
    fontFamily:    "'Inter', system-ui, sans-serif",
    particles: {
      count:  30,
      color:  '#1a1a1a',
      size:   [3, 8],
      speed:  [0.1, 0.4],
      opacity:[0.05, 0.2],
      shape:  'square',
    },
  },
};
