# ============================================================
#  routers/natural.py — Endpoint de traducción Q'eqchi'
# ============================================================
import subprocess, json, os
from fastapi import APIRouter, HTTPException
from pydantic import BaseModel

router = APIRouter()

CORE_BUILD = os.path.join(
    os.path.dirname(__file__), '..', '..', 'core', 'build'
)

class TranslateRequest(BaseModel):
    text: str

@router.post("/translate")
def translate(req: TranslateRequest):
    """Traduce texto Q'eqchi' al español usando el diccionario ALMG 2004."""
    binary = os.path.join(CORE_BUILD, 'natural_cli')
    if not os.path.exists(binary):
        raise HTTPException(503, f"Binario no encontrado: {binary}")

    result = subprocess.run(
        [binary, req.text],
        capture_output=True, text=True, timeout=10
    )

    raw = result.stdout.strip()
    if not raw:
        raise HTTPException(500, f"Sin respuesta del core: {result.stderr}")

    try:
        data = json.loads(raw)
    except json.JSONDecodeError:
        raise HTTPException(500, f"JSON invalido: {raw[:200]}")

    if not data.get('success'):
        raise HTTPException(400, data.get('error', 'Error en traduccion'))

    return data
