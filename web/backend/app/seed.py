# Khaali database mein synthetic students + achievements load karo, aur ek reviewer account banao.
# Chalao: python -m app.seed   (ya server pehli baar start hone pe apne aap)
import json
import logging
import secrets
from datetime import date, datetime

from sqlalchemy import func, insert, select

from .config import ADMIN_EMAIL, ADMIN_PASSWORD, CACHE_DIR, SEED_FILE
from .db import achievements, create_all, engine, reset_sequences, students
from .scoring import weighted_points
from .security import hash_password

log = logging.getLogger("forge.seed")


def _dt(v):
    return datetime.fromisoformat(v.replace("Z", "+00:00")).replace(tzinfo=None) if v else None


def ensure_admin(conn):
    if conn.execute(select(students.c.id).where(students.c.email == ADMIN_EMAIL)).first():
        return None
    password = ADMIN_PASSWORD or secrets.token_urlsafe(12)
    conn.execute(insert(students).values(
        roll_no="0000000001", name="Forge Reviewer", email=ADMIN_EMAIL, password_hash=hash_password(password),
        role="admin", course="Staff", branch="STAFF", campus="Dehradun", grad_year=2026, skills=[],
        show_profile=False, show_achievements=False, show_rank=False,
    ))
    if not ADMIN_PASSWORD:
        # Password sirf ek baar print hota hai; .cache mein bhi rakha taaki local demo mein mil jaye
        CACHE_DIR.mkdir(parents=True, exist_ok=True)
        (CACHE_DIR / "reviewer_password.txt").write_text(f"{ADMIN_EMAIL}\n{password}\n", encoding="utf-8")
        log.warning("Reviewer account %s created. Password saved in .cache/reviewer_password.txt", ADMIN_EMAIL)
    return password


def seed_if_empty():
    create_all()
    with engine.begin() as conn:
        has_students = conn.execute(select(func.count()).select_from(students).where(students.c.role == "student")).scalar()
        if not has_students:
            data = json.loads(SEED_FILE.read_text(encoding="utf-8"))
            srows = []
            for s in data["students"]:
                s = dict(s)
                s["created_at"] = _dt(s["created_at"])
                s["password_hash"] = None
                srows.append(s)
            conn.execute(insert(students), srows)
            arows = []
            for a in data["achievements"]:
                a = dict(a)
                a["achieved_on"] = date.fromisoformat(a["achieved_on"])
                a["submitted_at"] = _dt(a["submitted_at"])
                a["reviewed_at"] = _dt(a["reviewed_at"])
                a["points_applied"] = a["status"] == "approved"
                a["points"] = weighted_points(a["category"], a["score"], a["platform"]) if a["points_applied"] else None
                arows.append(a)
            conn.execute(insert(achievements), arows)
            reset_sequences(conn)
            log.info("Seeded %d students and %d achievements", len(srows), len(arows))
        ensure_admin(conn)


if __name__ == "__main__":
    logging.basicConfig(level=logging.INFO)
    seed_if_empty()
    print("Database ready.")
