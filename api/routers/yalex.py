# ============================================================
#  routers/yalex.py — Endpoints del analizador léxico
# ============================================================
import subprocess, json, tempfile, os
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel

router = APIRouter()

CORE_BUILD = os.path.join(os.path.dirname(__file__), "../../core/build")
YALEX_CLI  = os.path.join(CORE_BUILD, "yalex_cli")

class YAlexRequest(BaseModel):
    yalex_content: str
    input_text:    str

@router.post("/analyze")
def analyze(req: YAlexRequest):
    """
    Escribe .yalex e input en archivos temporales (evita problemas con
    caracteres especiales y saltos de línea en argumentos shell).
    """
    yalex_path = None
    input_path = None
    try:
        # Archivo temporal para el .yalex
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yalex',
                                         delete=False, encoding='utf-8') as f:
            f.write(req.yalex_content)
            yalex_path = f.name

        # Archivo temporal para el input (evita problemas con --string y saltos de línea)
        with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                         delete=False, encoding='utf-8') as f:
            f.write(req.input_text)
            input_path = f.name

        result = subprocess.run(
            [YALEX_CLI, yalex_path, input_path],
            capture_output=True, text=True, timeout=30
        )

        raw = result.stdout.strip()
        if not raw:
            raise HTTPException(400, result.stderr or "Sin respuesta del core")

        return json.loads(raw)

    except json.JSONDecodeError as e:
        raise HTTPException(500, f"JSON invalido del core: {str(e)}")
    except subprocess.TimeoutExpired:
        raise HTTPException(408, "Timeout")
    finally:
        if yalex_path and os.path.exists(yalex_path): os.unlink(yalex_path)
        if input_path and os.path.exists(input_path): os.unlink(input_path)
