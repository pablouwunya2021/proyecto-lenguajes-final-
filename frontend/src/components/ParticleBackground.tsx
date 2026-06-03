// ============================================================
//  ParticleBackground.tsx — Canvas animado con partículas
//  Se renderiza detrás del contenido y cambia según el tema
// ============================================================
import { useEffect, useRef } from 'react';
import type { ParticleConfig } from '../types/themes';

interface Particle {
  x: number; y: number;
  vx: number; vy: number;
  size: number;
  opacity: number;
  char: string;
  rotation: number;
  rotSpeed: number;
}

interface Props {
  config:   ParticleConfig | null;
  config2?: ParticleConfig | null;  // segunda capa (segundo color)
  // Para el tema Q'eqchi' ponemos un gradiente azul/blanco extra
  extraGradient?: string; // reserved for future use
}

export function ParticleBackground({ config, config2, extraGradient: _eg }: Props) {
  const canvasRef  = useRef<HTMLCanvasElement>(null);
  const canvas2Ref = useRef<HTMLCanvasElement>(null);

  // Capa 1
  useEffect(() => {
    if (!config || !canvasRef.current) return;
    const canvas  = canvasRef.current;
    const ctx     = canvas.getContext('2d')!;
    let animId:   number;
    let particles: Particle[] = [];

    const resize = () => {
      canvas.width  = canvas.offsetWidth;
      canvas.height = canvas.offsetHeight;
    };
    resize();
    window.addEventListener('resize', resize);

    const rand = (min: number, max: number) =>
      Math.random() * (max - min) + min;

    // Inicializar partículas
    const chars = config.char ? config.char.split('') : ['●'];
    for (let i = 0; i < config.count; i++) {
      particles.push({
        x:        rand(0, canvas.width),
        y:        rand(0, canvas.height),
        vx:       rand(-config.speed[1], config.speed[1]),
        vy:       rand(-config.speed[0], config.speed[1]),
        size:     rand(config.size[0], config.size[1]),
        opacity:  rand(config.opacity[0], config.opacity[1]),
        char:     chars[Math.floor(Math.random() * chars.length)],
        rotation: rand(0, Math.PI * 2),
        rotSpeed: rand(-0.01, 0.01),
      });
    }

    const draw = () => {
      ctx.clearRect(0, 0, canvas.width, canvas.height);

      particles.forEach(p => {
        ctx.save();
        ctx.globalAlpha = p.opacity;
        ctx.fillStyle   = config.color;
        ctx.strokeStyle = config.color;
        ctx.translate(p.x, p.y);
        ctx.rotate(p.rotation);

        if (config.shape === 'char') {
          ctx.font = `${p.size * 8}px "JetBrains Mono", monospace`;
          ctx.fillText(p.char, 0, 0);
        } else if (config.shape === 'circle') {
          ctx.beginPath();
          ctx.arc(0, 0, p.size, 0, Math.PI * 2);
          ctx.fill();
        } else if (config.shape === 'square') {
          ctx.fillRect(-p.size / 2, -p.size / 2, p.size, p.size);
        }

        ctx.restore();

        // Actualizar posición
        p.x += p.vx;
        p.y += p.vy;
        p.rotation += p.rotSpeed;

        // Rebote en los bordes
        if (p.x < -10)               p.x = canvas.width  + 10;
        if (p.x > canvas.width  + 10) p.x = -10;
        if (p.y < -10)               p.y = canvas.height + 10;
        if (p.y > canvas.height + 10) p.y = -10;
      });

      animId = requestAnimationFrame(draw);
    };

    draw();
    return () => {
      cancelAnimationFrame(animId);
      window.removeEventListener('resize', resize);
    };
  }, [config]);

  // Capa 2 (opcional — segundo color)
  useEffect(() => {
    if (!config2 || !canvas2Ref.current) return;
    const canvas  = canvas2Ref.current;
    const ctx     = canvas.getContext('2d')!;
    let animId:   number;
    let particles: Particle[] = [];

    const resize = () => {
      canvas.width  = canvas.offsetWidth;
      canvas.height = canvas.offsetHeight;
    };
    resize();
    window.addEventListener('resize', resize);

    const rand = (min: number, max: number) =>
      Math.random() * (max - min) + min;

    const chars = config2.char ? config2.char.split('') : ['●'];
    for (let i = 0; i < config2.count; i++) {
      particles.push({
        x:        rand(0, canvas.width),
        y:        rand(0, canvas.height),
        vx:       rand(-config2.speed[1], config2.speed[1]),
        vy:       rand(-config2.speed[0], config2.speed[1]),
        size:     rand(config2.size[0], config2.size[1]),
        opacity:  rand(config2.opacity[0], config2.opacity[1]),
        char:     chars[Math.floor(Math.random() * chars.length)],
        rotation: rand(0, Math.PI * 2),
        rotSpeed: rand(-0.01, 0.01),
      });
    }

    const draw = () => {
      ctx.clearRect(0, 0, canvas.width, canvas.height);
      particles.forEach(p => {
        ctx.save();
        ctx.globalAlpha = p.opacity;
        ctx.fillStyle   = config2.color;
        ctx.translate(p.x, p.y);
        ctx.rotate(p.rotation);
        if (config2.shape === 'char') {
          ctx.font = `${p.size * 8}px "JetBrains Mono", monospace`;
          ctx.fillText(p.char, 0, 0);
        } else if (config2.shape === 'circle') {
          ctx.beginPath();
          ctx.arc(0, 0, p.size, 0, Math.PI * 2);
          ctx.fill();
        } else if (config2.shape === 'square') {
          ctx.fillRect(-p.size / 2, -p.size / 2, p.size, p.size);
        }
        ctx.restore();
        p.x += p.vx;
        p.y += p.vy;
        p.rotation += p.rotSpeed;
        if (p.x < -10)                p.x = canvas.width  + 10;
        if (p.x > canvas.width  + 10) p.x = -10;
        if (p.y < -10)                p.y = canvas.height + 10;
        if (p.y > canvas.height + 10) p.y = -10;
      });
      animId = requestAnimationFrame(draw);
    };

    draw();
    return () => {
      cancelAnimationFrame(animId);
      window.removeEventListener('resize', resize);
    };
  }, [config2]);

  return (
    <>
      <canvas
        ref={canvasRef}
        className="absolute inset-0 w-full h-full pointer-events-none"
        style={{ zIndex: 0 }}
      />
      {config2 && (
        <canvas
          ref={canvas2Ref}
          className="absolute inset-0 w-full h-full pointer-events-none"
          style={{ zIndex: 0 }}
        />
      )}
    </>
  );
}

// ── Efecto scanlines para terminal ───────────────────────────
export function Scanlines() {
  return (
    <div
      className="absolute inset-0 pointer-events-none"
      style={{
        zIndex: 1,
        background: `repeating-linear-gradient(
          0deg,
          transparent,
          transparent 2px,
          rgba(0,0,0,0.08) 2px,
          rgba(0,0,0,0.08) 4px
        )`,
      }}
    />
  );
}
