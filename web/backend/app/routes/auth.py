from fastapi import APIRouter, Body, HTTPException
from sqlalchemy import func, insert, select

from ..config import DEMO_MODE
from ..db import engine, students
from ..engine import forge
from ..security import hash_password, make_token, verify_password
from ..validation import ValidationFailed, check_signup

router = APIRouter(prefix="/auth", tags=["Auth"])


def _session(row):
    return {"token": make_token(row["id"], row["role"]), "studentId": row["id"], "role": row["role"]}


@router.post("/signup", status_code=201)
def signup(body: dict = Body(...)):
    with engine.connect() as c:
        def taken(email):
            return c.execute(select(students.c.id).where(func.lower(students.c.email) == email)).first() is not None

        roll_taken = c.execute(select(students.c.id).where(students.c.roll_no == str(body.get("roll_no", "")).strip())).first()
        f = check_signup(body, taken)
    if roll_taken:
        raise ValidationFailed({"roll_no": "This roll number already has an account."})
    with engine.begin() as c:
        sid = c.execute(insert(students).values(
            roll_no=f["roll_no"], name=f["name"], email=f["email"], password_hash=hash_password(f["password"]),
            role="student", course="B.Tech", branch=f["branch"], campus=f["campus"], grad_year=f["grad_year"], skills=[],
        )).inserted_primary_key[0]
    forge.recompute()
    return _session({"id": sid, "role": "student"})


@router.post("/login")
def login(body: dict = Body(...)):
    email = str(body.get("email") or "").strip().lower()
    password = str(body.get("password") or "")
    if not email or not password:
        raise ValidationFailed({k: v for k, v in (("email", "Enter your email." if not email else None),
                                                  ("password", "Enter your password." if not password else None)) if v})
    with engine.connect() as c:
        row = c.execute(select(students).where(func.lower(students.c.email) == email)).mappings().first()
    # Same message dono cases mein, taaki koi emails guess na kar sake
    if row is None or not verify_password(password, row["password_hash"]):
        raise ValidationFailed({"password": "Email or password is wrong."})
    return _session(row)


@router.post("/demo")
def demo(body: dict = Body(default={})):
    if not DEMO_MODE:
        raise HTTPException(404, "Demo accounts are switched off.")
    with engine.connect() as c:
        if body.get("role") == "reviewer":
            row = c.execute(select(students).where(students.c.role == "admin").order_by(students.c.id)).mappings().first()
        else:
            forge.fresh()
            ranked = sorted(forge.board("all").items(), key=lambda kv: kv[1][1])
            ids = [sid for sid, v in ranked if v[1] >= 12]
            rows = {r["id"]: r for r in c.execute(select(students).where(students.c.id.in_(ids[:40]))).mappings()}
            row = next((rows[i] for i in ids if i in rows and rows[i]["show_rank"] and rows[i]["show_profile"]), None)
    if row is None:
        raise HTTPException(404, "No demo account is available.")
    return _session(row)
