# ============================================================
#  routers/yapar.py — Endpoints del analizador sintáctico
# ============================================================

import subprocess, json, tempfile, os
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel
from typing import Optional

router = APIRouter()

CORE_BUILD = os.path.join(os.path.dirname(__file__), "../../core/build")
YAPAR_CLI  = os.path.join(CORE_BUILD, "yapar_cli")
SPECS_DIR  = os.path.join(os.path.dirname(__file__), "../../core/examples/specs")

class YAParRequest(BaseModel):
    yapar_content: str           # Contenido del .yapar
    input_text:    str           # Texto a parsear
    yalex_content: Optional[str] = None  # .yalex para tokenizar (opcional)

@router.post("/analyze")
def analyze(req: YAParRequest):
    """
    Construye el autómata LR(0), las tablas SLR/LL1/LALR,
    resuelve conflictos en paralelo y parsea el input.
    """
    # Archivos temporales para .yapar y .yalex
    with tempfile.NamedTemporaryFile(mode='w', suffix='.yapar',
                                     delete=False) as f:
        f.write(req.yapar_content)
        yapar_path = f.name

    yalex_path = None
    if req.yalex_content:
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yalex',
                                         delete=False) as f:
            f.write(req.yalex_content)
            yalex_path = f.name

    try:
        cmd = [YAPAR_CLI, yapar_path, "--string", req.input_text]
        if yalex_path:
            cmd.append(yalex_path)

        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=30
        )

        if result.returncode != 0 and not result.stdout:
            raise HTTPException(status_code=400,
                                detail=result.stderr or "Error en yapar_cli")

        return json.loads(result.stdout)

    except json.JSONDecodeError:
        raise HTTPException(status_code=500,
                            detail="Respuesta inválida del core")
    except subprocess.TimeoutExpired:
        raise HTTPException(status_code=408, detail="Timeout")
    finally:
        os.unlink(yapar_path)
        if yalex_path:
            os.unlink(yalex_path)

@router.get("/examples")
def list_examples():
    files = [f for f in os.listdir(SPECS_DIR) if f.endswith('.yapar')]
    return {"files": files}

@router.get("/example/{filename}")
def get_example(filename: str):
    path = os.path.join(SPECS_DIR, filename)
    if not os.path.exists(path):
        raise HTTPException(status_code=404, detail="Archivo no encontrado")
    with open(path) as f:
        return {"content": f.read(), "filename": filename}
