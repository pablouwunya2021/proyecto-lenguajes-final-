// ============================================================
//  TreeView.tsx — Árbol sintáctico jerárquico con D3.js
// ============================================================
import { useEffect, useRef } from 'react';
import * as d3 from 'd3';
import { useStore } from '../../store/useStore';
import type { ParseNode } from '../../types';

interface D3TreeNode extends d3.HierarchyPointNode<ParseNode> {}

export function TreeView() {
  const { yaparResult } = useStore();
  const svgRef = useRef<SVGSVGElement>(null);

  useEffect(() => {
    if (!yaparResult?.tree || !svgRef.current) return;

    const svg = d3.select(svgRef.current);
    svg.selectAll('*').remove();

    const W = svgRef.current.clientWidth  || 800;
    const H = svgRef.current.clientHeight || 500;
    const margin = { top: 40, right: 40, bottom: 40, left: 40 };

    const g = svg.append('g');
    svg.call(
      d3.zoom<SVGSVGElement, unknown>()
        .scaleExtent([0.2, 3])
        .on('zoom', (e) => g.attr('transform', e.transform))
    );

    // Convertir el árbol JSON a jerarquía D3
    const root = d3.hierarchy<ParseNode>(yaparResult.tree, d => d.children);

    // Layout del árbol (vertical)
    const treeLayout = d3.tree<ParseNode>()
      .size([W - margin.left - margin.right, H - margin.top - margin.bottom])
      .separation((a, b) => (a.parent === b.parent ? 1.2 : 1.8));

    const treeData = treeLayout(root);

    // Color por tipo de nodo
    const nodeColor = (d: D3TreeNode): string => {
      if (!d.children && !d.data.children) {
        // Hoja (terminal)
        return d.data.lexeme ? '#7c3aed' : '#0891b2';
      }
      // No-terminal
      return '#1c2128';
    };

    const borderColor = (d: D3TreeNode): string => {
      if (!d.children && !d.data.children) return '#a855f7';
      return '#30363d';
    };

    // Dibujar enlaces curvos
    g.selectAll('.link')
      .data(treeData.links())
      .enter().append('path')
      .attr('class', 'link')
      .attr('fill', 'none')
      .attr('stroke', '#30363d')
      .attr('stroke-width', 1.5)
      .attr('d', d3.linkVertical<d3.HierarchyPointLink<ParseNode>, d3.HierarchyPointNode<ParseNode>>()
        .x(d => d.x + margin.left)
        .y(d => d.y + margin.top)
      );

    // Dibujar nodos
    const node = g.selectAll('.node')
      .data(treeData.descendants())
      .enter().append('g')
      .attr('class', 'node')
      .attr('transform', d => `translate(${d.x + margin.left},${d.y + margin.top})`);

    const isLeaf = (d: D3TreeNode) => !d.children && !d.data.children;

    // Fondo del nodo
    node.append('rect')
      .attr('x', -30).attr('y', -14)
      .attr('width', 60).attr('height', 28)
      .attr('rx', 6).attr('ry', 6)
      .attr('fill', d => nodeColor(d as D3TreeNode))
      .attr('stroke', d => borderColor(d as D3TreeNode))
      .attr('stroke-width', 1.5);

    // Texto del símbolo
    node.append('text')
      .attr('text-anchor', 'middle')
      .attr('dy', '0.35em')
      .attr('fill', d => isLeaf(d as D3TreeNode) ? '#c9d1d9' : '#8b949e')
      .attr('font-size', '11px')
      .attr('font-family', 'JetBrains Mono, monospace')
      .attr('font-weight', d => isLeaf(d as D3TreeNode) ? '600' : '400')
      .text(d => (d.data as ParseNode).name);

    // Lexema debajo del nodo (solo hojas con valor)
    node.filter(d => !!(d.data as ParseNode).lexeme)
      .append('text')
      .attr('text-anchor', 'middle')
      .attr('dy', '28px')
      .attr('fill', '#f59e0b')
      .attr('font-size', '10px')
      .attr('font-family', 'JetBrains Mono, monospace')
      .text(d => `"${(d.data as ParseNode).lexeme}"`);

    // Animación de entrada
    node.attr('opacity', 0)
      .transition().duration(400)
      .delay((_, i) => i * 20)
      .attr('opacity', 1);

  }, [yaparResult]);

  if (!yaparResult?.tree) {
    return (
      <div className="flex items-center justify-center h-full text-text-secondary">
        <div className="text-center">
          <div className="text-5xl mb-4">🌳</div>
          <p className="text-lg font-medium">El árbol sintáctico aparecerá aquí</p>
          <p className="text-sm mt-2 text-text-muted">Ejecuta el parser con un input válido</p>
        </div>
      </div>
    );
  }

  return <svg ref={svgRef} className="w-full h-full" />;
}
