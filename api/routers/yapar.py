# ============================================================
#  routers/yapar.py — Endpoints del analizador sintáctico
# ============================================================
import subprocess, json, tempfile, os
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel

router = APIRouter()

CORE_BUILD = os.path.join(os.path.dirname(__file__), "../../core/build")
YAPAR_CLI  = os.path.join(CORE_BUILD, "yapar_cli")

class YAParRequest(BaseModel):
    yapar_content: str
    yalex_content: str = ""
    input_text:    str = ""

@router.post("/analyze")
def analyze(req: YAParRequest):
    """Analiza gramática .yapar + tokeniza con .yalex si se provee."""
    yapar_path = yalex_path = input_path = None
    try:
        with tempfile.NamedTemporaryFile(mode='w', suffix='.yapar',
                                         delete=False, encoding='utf-8') as f:
            f.write(req.yapar_content)
            yapar_path = f.name

        cmd = [YAPAR_CLI, yapar_path]

        if req.input_text:
            with tempfile.NamedTemporaryFile(mode='w', suffix='.txt',
                                             delete=False, encoding='utf-8') as f:
                f.write(req.input_text)
                input_path = f.name
            cmd.append(input_path)
        else:
            cmd += ["--string", ""]

        if req.yalex_content:
            with tempfile.NamedTemporaryFile(mode='w', suffix='.yalex',
                                             delete=False, encoding='utf-8') as f:
                f.write(req.yalex_content)
                yalex_path = f.name
            cmd.append(yalex_path)

        result = subprocess.run(
            cmd, capture_output=True, text=True, timeout=30
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
        for p in [yapar_path, yalex_path, input_path]:
            if p and os.path.exists(p): os.unlink(p)
