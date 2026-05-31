/** @type {import('tailwindcss').Config} */
export default {
  content: ["./index.html", "./src/**/*.{js,ts,jsx,tsx}"],
  theme: {
    extend: {
      colors: {
        // Paleta oscura tipo IDE profesional
        bg:       { DEFAULT: '#0d1117', secondary: '#161b22', card: '#1c2128' },
        border:   { DEFAULT: '#30363d', light: '#3d444d' },
        accent:   { purple: '#7c3aed', teal: '#0891b2', green: '#10b981',
                    orange: '#f59e0b', red: '#ef4444', pink: '#ec4899' },
        text:     { primary: '#e6edf3', secondary: '#8b949e', muted: '#484f58' },
      },
      fontFamily: {
        mono: ['JetBrains Mono', 'Fira Code', 'monospace'],
        sans: ['Inter', 'system-ui', 'sans-serif'],
      },
      animation: {
        'fade-in':    'fadeIn 0.3s ease-in-out',
        'slide-up':   'slideUp 0.4s ease-out',
        'pulse-slow': 'pulse 3s cubic-bezier(0.4,0,0.6,1) infinite',
        'glow':       'glow 2s ease-in-out infinite alternate',
      },
      keyframes: {
        fadeIn:  { from: { opacity: 0 }, to: { opacity: 1 } },
        slideUp: { from: { opacity: 0, transform: 'translateY(10px)' },
                   to:   { opacity: 1, transform: 'translateY(0)' } },
        glow:    { from: { boxShadow: '0 0 5px #7c3aed44' },
                   to:   { boxShadow: '0 0 20px #7c3aed88, 0 0 40px #7c3aed44' } },
      },
    },
  },
  plugins: [],
}
