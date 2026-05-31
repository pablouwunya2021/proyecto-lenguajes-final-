// ============================================================
//  AutomatonView.tsx — Visualización del autómata LR(0) con D3.js
//
//  Los nodos se colocan en un layout de fuerza (force-directed).
//  - Nodos azules: estados normales
//  - Nodos verdes con brillo: estados de aceptación
//  - El estado inicial tiene una flecha de entrada
//  - Al hover sobre un nodo se muestran sus ítems LR(0)
// ============================================================
import { useEffect, useRef, useState } from 'react';
import * as d3 from 'd3';
import { useStore } from '../../store/useStore';
import type { LR0State } from '../../types';

interface D3Node extends d3.SimulationNodeDatum {
  id: number;
  accepting: boolean;
  items: string[];
}

interface D3Link extends d3.SimulationLinkDatum<D3Node> {
  label: string;
}

export function AutomatonView() {
  const { yaparResult } = useStore();
  const svgRef  = useRef<SVGSVGElement>(null);
  const [hovered, setHovered] = useState<LR0State | null>(null);

  useEffect(() => {
    if (!yaparResult?.automaton || !svgRef.current) return;

    const { states, transitions } = yaparResult.automaton;
    const svg = d3.select(svgRef.current);
    svg.selectAll('*').remove();

    const W = svgRef.current.clientWidth  || 800;
    const H = svgRef.current.clientHeight || 500;

    // Fondo con gradiente sutil
    const defs = svg.append('defs');

    // Gradiente para nodos normales
    const grad = defs.append('radialGradient').attr('id', 'node-grad');
    grad.append('stop').attr('offset', '0%').attr('stop-color', '#1c2128');
    grad.append('stop').attr('offset', '100%').attr('stop-color', '#161b22');

    // Gradiente para nodos de aceptación
    const gradAcc = defs.append('radialGradient').attr('id', 'node-accept-grad');
    gradAcc.append('stop').attr('offset', '0%').attr('stop-color', '#065f46');
    gradAcc.append('stop').attr('offset', '100%').attr('stop-color', '#022c22');

    // Marcador de flecha
    defs.append('marker')
      .attr('id', 'arrow')
      .attr('viewBox', '0 -5 10 10')
      .attr('refX', 28).attr('refY', 0)
      .attr('markerWidth', 6).attr('markerHeight', 6)
      .attr('orient', 'auto')
      .append('path')
      .attr('d', 'M0,-5L10,0L0,5')
      .attr('fill', '#484f58');

    // Marcador flecha para self-loops
    defs.append('marker')
      .attr('id', 'arrow-self')
      .attr('viewBox', '0 -5 10 10')
      .attr('refX', 10).attr('refY', 0)
      .attr('markerWidth', 6).attr('markerHeight', 6)
      .attr('orient', 'auto')
      .append('path')
      .attr('d', 'M0,-5L10,0L0,5')
      .attr('fill', '#484f58');

    // Zoom y pan
    const g = svg.append('g');
    svg.call(
      d3.zoom<SVGSVGElement, unknown>()
        .scaleExtent([0.3, 3])
        .on('zoom', (e) => g.attr('transform', e.transform))
    );

    // Preparar nodos y links para D3
    const nodes: D3Node[] = states.map(s => ({
      id: s.id, accepting: s.accepting, items: s.items,
    }));

    // Agrupar transiciones paralelas (mismo from→to con diferentes labels)
    const linkMap = new Map<string, D3Link & { labels: string[] }>();
    transitions.forEach(t => {
      const key = `${t.from}-${t.to}`;
      if (!linkMap.has(key)) {
        linkMap.set(key, {
          source: t.from, target: t.to,
          label: t.label, labels: [t.label]
        });
      } else {
        linkMap.get(key)!.labels.push(t.label);
        linkMap.get(key)!.label = linkMap.get(key)!.labels.join(', ');
      }
    });
    const links = Array.from(linkMap.values());

    // Simulación de fuerzas
    const sim = d3.forceSimulation<D3Node>(nodes)
      .force('link', d3.forceLink<D3Node, D3Link>(links)
        .id(d => d.id).distance(120).strength(0.5))
      .force('charge', d3.forceManyBody().strength(-400))
      .force('center', d3.forceCenter(W / 2, H / 2))
      .force('collision', d3.forceCollide(45));

    // Dibujar enlaces
    const link = g.selectAll('.link')
      .data(links).enter().append('g').attr('class', 'link');

    link.append('line')
      .attr('stroke', '#30363d')
      .attr('stroke-width', 1.5)
      .attr('marker-end', 'url(#arrow)');

    link.append('text')
      .attr('fill', '#8b949e')
      .attr('font-size', '11px')
      .attr('font-family', 'JetBrains Mono, monospace')
      .attr('text-anchor', 'middle')
      .text(d => d.label.length > 15 ? d.label.substring(0, 12) + '...' : d.label);

    // Dibujar nodos
    const node = g.selectAll('.node')
      .data(nodes).enter().append('g')
      .attr('class', 'node')
      .style('cursor', 'pointer')
      .call(
        d3.drag<SVGGElement, D3Node>()
          .on('start', (e, d) => { if (!e.active) sim.alphaTarget(0.3).restart(); d.fx=d.x; d.fy=d.y; })
          .on('drag',  (e, d) => { d.fx=e.x; d.fy=e.y; })
          .on('end',   (e, d) => { if (!e.active) sim.alphaTarget(0); d.fx=null; d.fy=null; })
      );

    // Círculo exterior (brillo para aceptación)
    node.filter(d => d.accepting)
      .append('circle')
      .attr('r', 28)
      .attr('fill', 'none')
      .attr('stroke', '#10b981')
      .attr('stroke-width', 1.5)
      .attr('stroke-dasharray', '4,2')
      .attr('opacity', 0.6);

    // Círculo principal
    node.append('circle')
      .attr('r', 22)
      .attr('fill', d => d.accepting ? 'url(#node-accept-grad)' : 'url(#node-grad)')
      .attr('stroke', d => d.accepting ? '#10b981' : '#7c3aed')
      .attr('stroke-width', d => d.accepting ? 2 : 1.5)
      .on('mouseover', (_, d) => {
        setHovered(states.find(s => s.id === d.id) || null);
      })
      .on('mouseout', () => setHovered(null));

    // Texto del ID del estado
    node.append('text')
      .attr('fill', d => d.accepting ? '#10b981' : '#c9d1d9')
      .attr('font-size', '13px')
      .attr('font-weight', '600')
      .attr('font-family', 'JetBrains Mono, monospace')
      .attr('text-anchor', 'middle')
      .attr('dy', '0.35em')
      .text(d => `I${d.id}`)
      .style('pointer-events', 'none');

    // Flecha de entrada al estado inicial
    const startNode = nodes.find(n => n.id === 0);
    if (startNode) {
      g.append('line')
        .attr('class', 'start-arrow')
        .attr('stroke', '#7c3aed')
        .attr('stroke-width', 2)
        .attr('marker-end', 'url(#arrow)');
    }

    // Actualizar posiciones en cada tick
    sim.on('tick', () => {
      link.select('line')
        .attr('x1', d => (d.source as D3Node).x!)
        .attr('y1', d => (d.source as D3Node).y!)
        .attr('x2', d => (d.target as D3Node).x!)
        .attr('y2', d => (d.target as D3Node).y!);

      link.select('text')
        .attr('x', d => ((d.source as D3Node).x! + (d.target as D3Node).x!) / 2)
        .attr('y', d => ((d.source as D3Node).y! + (d.target as D3Node).y!) / 2 - 6);

      node.attr('transform', d => `translate(${d.x},${d.y})`);

      // Flecha inicial
      if (startNode) {
        g.select('.start-arrow')
          .attr('x1', (startNode.x ?? 0) - 60)
          .attr('y1', startNode.y ?? 0)
          .attr('x2', (startNode.x ?? 0) - 25)
          .attr('y2', startNode.y ?? 0);
      }
    });

    return () => { sim.stop(); };
  }, [yaparResult]);

  if (!yaparResult) {
    return (
      <div className="flex items-center justify-center h-full text-text-secondary">
        <div className="text-center">
          <div className="text-5xl mb-4">🔷</div>
          <p className="text-lg font-medium">Ejecuta YAPar para visualizar el autómata</p>
          <p className="text-sm mt-2 text-text-muted">El autómata LR(0) aparecerá aquí</p>
        </div>
      </div>
    );
  }

  return (
    <div className="relative w-full h-full">
      <svg ref={svgRef} className="w-full h-full" />

      {/* Panel de información del estado hover */}
      {hovered && (
        <div className="absolute top-4 right-4 card max-w-xs animate-fade-in z-10">
          <div className="flex items-center gap-2 mb-2">
            <span className={`w-3 h-3 rounded-full ${hovered.accepting ? 'bg-accent-green' : 'bg-accent-purple'}`} />
            <span className="font-mono font-bold text-text-primary">Estado I{hovered.id}</span>
            {hovered.accepting && <span className="text-xs text-accent-green">ACEPTA</span>}
          </div>
          <div className="space-y-1">
            {hovered.items.slice(0, 6).map((item, i) => (
              <div key={i} className="text-xs font-mono text-text-secondary bg-bg-secondary px-2 py-1 rounded">
                {item}
              </div>
            ))}
            {hovered.items.length > 6 && (
              <div className="text-xs text-text-muted">+{hovered.items.length - 6} más...</div>
            )}
          </div>
        </div>
      )}

      {/* Leyenda */}
      <div className="absolute bottom-4 left-4 flex items-center gap-4 text-xs text-text-secondary">
        <div className="flex items-center gap-1.5">
          <div className="w-3 h-3 rounded-full bg-bg-card border border-accent-purple" />
          <span>Estado</span>
        </div>
        <div className="flex items-center gap-1.5">
          <div className="w-3 h-3 rounded-full bg-bg-card border-2 border-accent-green" />
          <span>Aceptación</span>
        </div>
        <div className="text-text-muted">Scroll: zoom · Drag: mover</div>
      </div>
    </div>
  );
}
