# GEHU Forge API. Chalao: uvicorn app.main:app --reload --port 8000   (web/backend folder se)
import logging
from contextlib import asynccontextmanager

from fastapi import FastAPI, HTTPException, Request
from fastapi.exceptions import RequestValidationError
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import JSONResponse

from .config import CORS_ORIGINS, JWT_SECRET_IS_RANDOM
from .engine import EngineError, forge
from .routes import achievements, admin, auth, students
from .seed import seed_if_empty
from .validation import ValidationFailed

logging.basicConfig(level=logging.INFO, format="%(levelname)s %(name)s: %(message)s")
log = logging.getLogger("forge")


@asynccontextmanager
async def lifespan(app):
    seed_if_empty()
    forge.start()
    forge.recompute()
    if JWT_SECRET_IS_RANDOM:
        log.warning("JWT_SECRET is not set: sign-ins reset when the server restarts. Set it in .env.")
    yield
    forge.stop()


app = FastAPI(title="GEHU Forge API", version="1.0.0", lifespan=lifespan,
              description="Students, achievements, reviewer queue. Ranking and search run in the C++ engine.")
app.add_middleware(CORSMiddleware, allow_origins=CORS_ORIGINS, allow_credentials=False,
                   allow_methods=["GET", "POST", "PUT", "DELETE"], allow_headers=["Authorization", "Content-Type"])

for r in (auth.router, students.router, achievements.router, admin.router):
    app.include_router(r, prefix="/api")


@app.get("/api/health")
def health():
    return {"ok": True, "engine": forge.proc is not None and forge.proc.poll() is None, "ranked": forge.counts.get("all", 0)}


# Saare errors ek hi shape mein: {"error": {code, message, field, errors}}
CODES = {401: "unauthorized", 403: "forbidden", 404: "not_found", 409: "already_reviewed", 422: "validation_error"}


def _error(status, code, message, field=None, errors=None):
    return JSONResponse(status_code=status, content={"error": {"code": code, "message": message, "field": field, "errors": errors}})


@app.exception_handler(ValidationFailed)
async def on_validation(_: Request, exc: ValidationFailed):
    field = next(iter(exc.errors), None)
    return _error(422, "validation_error", exc.errors[field] if field else str(exc), field, exc.errors)


@app.exception_handler(RequestValidationError)
async def on_request_validation(_: Request, exc: RequestValidationError):
    first = exc.errors()[0] if exc.errors() else {}
    field = str(first.get("loc", [""])[-1]) or None
    return _error(422, "validation_error", first.get("msg", "Invalid input."), field)


@app.exception_handler(HTTPException)
async def on_http(_: Request, exc: HTTPException):
    msg = exc.detail if isinstance(exc.detail, str) else "Request failed."
    return _error(exc.status_code, CODES.get(exc.status_code, "request_error"), msg)


@app.exception_handler(EngineError)
async def on_engine(_: Request, exc: EngineError):
    log.error("Engine error: %s", exc)
    return _error(503, "engine_unavailable", "Rankings are temporarily unavailable. Try again in a moment.")
