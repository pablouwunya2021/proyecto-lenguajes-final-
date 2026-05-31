# ============================================================
#  routers/yalex.py — Endpoints del analizador léxico
# ============================================================

import subprocess, json, tempfile, os
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel

router = APIRouter()

# Ruta al binario compilado (relativa al api/)
CORE_BUILD = os.path.join(os.path.dirname(__file__), "../../core/build")
YALEX_CLI  = os.path.join(CORE_BUILD, "yalex_cli")
SPECS_DIR  = os.path.join(os.path.dirname(__file__), "../../core/examples/specs")

class YAlexRequest(BaseModel):
    yalex_content: str   # Contenido del archivo .yalex
    input_text:    str   # Texto a tokenizar

class YAlexFileRequest(BaseModel):
    yalex_filename: str  # Nombre de archivo predefinido (ejemplo.yalex)
    input_text:     str

@router.post("/analyze")
def analyze(req: YAlexRequest):
    """
    Recibe el contenido de un .yalex y un texto a tokenizar.
    Escribe el .yalex en un archivo temporal, llama yalex_cli y devuelve el JSON.
    """
    # Escribimos el .yalex en un archivo temporal
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yalex',
                                     delete=False) as f:
        f.write(req.yalex_content)
        yalex_path = f.name

    try:
        result = subprocess.run(
            [YALEX_CLI, yalex_path, "--string", req.input_text],
            capture_output=True, text=True, timeout=30
        )
        # El binario imprime logs a stderr y JSON a stdout
        if result.returncode != 0 and not result.stdout:
            raise HTTPException(status_code=400,
                                detail=result.stderr or "Error en yalex_cli")
        return json.loads(result.stdout)
    except json.JSONDecodeError:
        raise HTTPException(status_code=500,
                            detail="Respuesta inválida del core")
    except subprocess.TimeoutExpired:
        raise HTTPException(status_code=408, detail="Timeout")
    finally:
        os.unlink(yalex_path)

@router.get("/examples")
def list_examples():
    """Lista los archivos .yalex de ejemplo disponibles"""
    files = [f for f in os.listdir(SPECS_DIR) if f.endswith('.yalex')]
    return {"files": files}

@router.get("/example/{filename}")
def get_example(filename: str):
    """Devuelve el contenido de un archivo .yalex de ejemplo"""
    path = os.path.join(SPECS_DIR, filename)
    if not os.path.exists(path):
        raise HTTPException(status_code=404, detail="Archivo no encontrado")
    with open(path) as f:
        return {"content": f.read(), "filename": filename}
