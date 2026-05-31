# ============================================================
#  main.py — FastAPI server
#  Puente entre el frontend React y los binarios C++
#
#  Endpoints principales:
#    POST /yalex/analyze   → llama yalex_cli
#    POST /yapar/analyze   → llama yapar_cli
#
#  Los binarios leen/escriben JSON por stdout/stdin.
# ============================================================

from fastapi import FastAPI
from fastapi.middleware.cors import CORSMiddleware
from routers import yalex, yapar

app = FastAPI(
    title="Generador de Analizadores Sintácticos",
    description="API para YALex, YAPar, parser maya Q'eqchi' y Xinca",
    version="1.0.0"
)

# Permitimos requests desde el frontend React (puerto 5173 por defecto de Vite)
app.add_middleware(
    CORSMiddleware,
    allow_origins=["http://localhost:5173", "http://localhost:3000"],
    allow_methods=["*"],
    allow_headers=["*"],
)

app.include_router(yalex.router, prefix="/yalex", tags=["YALex"])
app.include_router(yapar.router, prefix="/yapar", tags=["YAPar"])

@app.get("/")
def root():
    return {"status": "ok", "message": "Generador de Analizadores Sintácticos API"}
