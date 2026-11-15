# GEHU Forge API

FastAPI backend for the website. Ranking and student search run in the C++ engine (`engine/forge_engine.cpp`), which the API keeps running as a child process.

```
React (web/frontend) --/api--> FastAPI (web/backend) --> SQLite or Supabase Postgres
                                      |
                                      +--stdin/stdout--> C++ engine: AVL leaderboards, Trie + heap search
```

## Run locally (Windows PowerShell)

```powershell
cd web/backend
python -m venv .venv
.venv\Scripts\activate
pip install -r requirements.txt
copy .env.example .env          # then set JWT_SECRET
uvicorn app.main:app --reload --port 8000
```

On first start the API creates the tables, loads 250 synthetic students from `seed/forge.json`, builds the C++ engine with `g++` if `engine/forge_engine.exe` is missing, and creates the reviewer account. If `ADMIN_PASSWORD` is empty, a random password is written to `.cache/reviewer_password.txt`.

Frontend, in a second terminal:

```powershell
cd web/frontend
npm install
npm run dev                     # http://localhost:5173, /api is proxied to port 8000
```

API docs: http://localhost:8000/docs

## Tests

```powershell
pytest -q                        # API flow on a temporary database with the real C++ engine
```

## Switch to Supabase

Set `DATABASE_URL` in `.env` to the Supabase Postgres connection string (`postgresql+psycopg://…`). Restart the API: it creates the tables and seeds them. Nothing else changes.

## Endpoints

| Method | Path | Who |
|---|---|---|
| POST | `/api/auth/signup`, `/api/auth/login`, `/api/auth/demo` | anyone |
| GET | `/api/landing` | anyone |
| GET/PUT | `/api/me` | student |
| GET | `/api/dashboard`, `/api/leaderboard?category=&q=&branch=&batch=&campus=&tier=` | signed in |
| GET | `/api/students/{id}` | anyone (privacy applied) |
| POST/PUT/DELETE | `/api/achievements`, `/api/achievements/{id}` | student (pending only for PUT/DELETE) |
| GET | `/api/admin/pending?page=&category=`, `/api/admin/stats` | reviewer |
| POST | `/api/admin/review/{id}` `{decision, note, score}`, `/api/admin/recheck/{id}` | reviewer |

## Verification rules

- A submission starts as `pending`. Only `pending` can change, and the update is conditional (`WHERE status = 'pending'`), so two reviewers cannot both decide it.
- EXP is recomputed by the C++ engine from approved rows only, so an achievement can never be counted twice.
- Each student visit is recorded once per day in `activity`. The engine uses it for the consistency bonus (visit streak, active days), together with weekly achievement streaks.
- If the C++ process crashes, the API restarts it and replays the last `LOAD` before the next command.
- Rejecting needs a note. The reviewer can lower the claimed points before approving.
- Automatic checks use only free public APIs: Codeforces (`user.rating`, no key) and GitHub (pull requests and repositories, 60 requests per hour without a token). Student links are never fetched directly.
