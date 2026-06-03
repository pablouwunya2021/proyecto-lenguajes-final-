# ============================================================
#  routers/graphviz.py — Convierte DOT a SVG/PNG usando Graphviz
# ============================================================
import subprocess, tempfile, os
from fastapi import APIRouter, HTTPException
from fastapi.responses import Response
from pydantic import BaseModel

router = APIRouter()

# Formatos de salida permitidos y su media-type asociado
FORMATS = {
    "svg": "image/svg+xml",
    "png": "image/png",
}

class DotRequest(BaseModel):
    dot:      str            # Contenido del archivo .dot
    engine:   str  = "dot"   # dot, neato, fdp, circo, twopi
    format:   str  = "svg"   # svg | png
    download: bool = False    # si True, fuerza la descarga (Content-Disposition)


def _find_dot_binary() -> str:
    """Localiza el binario `dot` en las rutas comunes (Mac M5 / Linux)."""
    for path in ['/opt/homebrew/bin/dot', '/usr/local/bin/dot', '/usr/bin/dot', 'dot']:
        try:
            result = subprocess.run([path, '-V'], capture_output=True, timeout=2)
            if result.returncode in (0, 1):
                return path
        except FileNotFoundError:
            continue
    raise HTTPException(503, "Graphviz (dot) no está instalado")


@router.post("/render", response_class=Response)
def render(req: DotRequest):
    """
    Convierte un grafo Graphviz DOT a SVG o PNG.

    - format="svg" (por defecto): ideal para mostrar en pantalla (grafos pequeños).
    - format="png": ideal para descargar grafos grandes que el navegador no
      puede renderizar como SVG inline.
    - download=True: añade Content-Disposition para que el navegador lo descargue.
    """
    # Validar engine para evitar inyección de comandos
    allowed_engines = {"dot", "neato", "fdp", "circo", "twopi", "sfdp"}
    if req.engine not in allowed_engines:
        raise HTTPException(400, f"Engine no permitido: {req.engine}")

    fmt = req.format.lower()
    if fmt not in FORMATS:
        raise HTTPException(400, f"Formato no permitido: {req.format} (usa svg o png)")

    # El timeout escala con el tamaño del grafo: grafos grandes (muchos estados)
    # generan archivos .dot enormes que `dot` tarda más en procesar.
    # Base 15s + 1s por cada ~2KB de DOT, con tope de 120s.
    timeout_s = min(120, max(15, len(req.dot) // 2000))

    dot_path = None
    try:
        # Escribir .dot en archivo temporal
        with tempfile.NamedTemporaryFile(mode='w', suffix='.dot',
                                         delete=False, encoding='utf-8') as f:
            f.write(req.dot)
            dot_path = f.name

        dot_binary = _find_dot_binary()

        # Convertir DOT → SVG/PNG
        cmd = [dot_binary, f'-T{fmt}']
        # Para PNG limitamos el DPI para que el archivo no sea gigantesco
        # cuando el autómata tiene cientos de estados.
        if fmt == "png":
            cmd += ['-Gdpi=96']
        cmd.append(dot_path)

        result = subprocess.run(cmd, capture_output=True, timeout=timeout_s)

        if result.returncode != 0:
            err = result.stderr.decode('utf-8', errors='replace')
            raise HTTPException(400, f"Error en Graphviz: {err[:300]}")

        headers = {"Cache-Control": "no-cache"}
        if req.download:
            headers["Content-Disposition"] = f'attachment; filename="automata.{fmt}"'

        return Response(
            content=result.stdout,
            media_type=FORMATS[fmt],
            headers=headers,
        )

    except subprocess.TimeoutExpired:
        raise HTTPException(
            408,
            f"Graphviz timeout ({timeout_s}s) — el grafo es demasiado grande. "
            "Intenta descargarlo como PNG."
        )
    finally:
        if dot_path and os.path.exists(dot_path):
            os.unlink(dot_path)
