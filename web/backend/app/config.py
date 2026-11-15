import os
import secrets
from pathlib import Path

from dotenv import load_dotenv

BACKEND_DIR = Path(__file__).resolve().parents[1]
REPO_DIR = BACKEND_DIR.parents[1]
load_dotenv(BACKEND_DIR / ".env")


def _bool(name, default):
    return os.getenv(name, str(default)).strip().lower() in {"1", "true", "yes", "on"}


# Local default SQLite hai; Supabase ke liye uska Postgres connection string daal do
DATABASE_URL = os.getenv("DATABASE_URL", f"sqlite:///{(BACKEND_DIR / 'forge.db').as_posix()}")

# Secret na mile toh har restart pe naya banega (tokens invalid ho jayenge) - production mein set karo
JWT_SECRET = os.getenv("JWT_SECRET") or secrets.token_urlsafe(48)
JWT_SECRET_IS_RANDOM = not os.getenv("JWT_SECRET")
TOKEN_DAYS = int(os.getenv("TOKEN_DAYS", "7"))

CORS_ORIGINS = [o.strip() for o in os.getenv("CORS_ORIGINS", "http://localhost:5173").split(",") if o.strip()]

# Demo mode: "demo account" aur "demo reviewer" buttons. Production mein false
DEMO_MODE = _bool("DEMO_MODE", True)

ADMIN_EMAIL = os.getenv("ADMIN_EMAIL", "reviewer@gehu.ac.in").lower()
ADMIN_PASSWORD = os.getenv("ADMIN_PASSWORD")

ENGINE_PATH = Path(os.getenv("ENGINE_PATH", REPO_DIR / "engine" / ("forge_engine.exe" if os.name == "nt" else "forge_engine")))
ENGINE_SOURCE = REPO_DIR / "engine" / "forge_engine.cpp"
CACHE_DIR = BACKEND_DIR / ".cache"
SEED_FILE = BACKEND_DIR / "seed" / "forge.json"

# Free APIs: GitHub (bina token 60 req/hour), Codeforces (bina key)
GITHUB_TOKEN = os.getenv("GITHUB_TOKEN")
AUTO_CHECKS = _bool("AUTO_CHECKS", True)
