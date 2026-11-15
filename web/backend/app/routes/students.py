from fastapi import APIRouter, Depends, HTTPException, Query
from sqlalchemy import select, update

from .. import views
from ..db import engine, students, utcnow
from ..engine import forge
from ..scoring import CATEGORY
from ..security import current_user, optional_user
from ..validation import check_profile

router = APIRouter(tags=["Students"])


def _student_only(user):
    if user["role"] != "student":
        raise HTTPException(403, "Reviewer accounts do not have a student profile.")


@router.get("/landing")
def landing():
    forge.fresh()
    return views.landing()


@router.get("/me")
def me(user=Depends(current_user)):
    forge.fresh()
    with engine.connect() as c:
        s = c.execute(select(students).where(students.c.id == user["id"])).mappings().first()
    if user["role"] == "admin":
        return {"id": s["id"], "name": s["name"], "email": s["email"], "role": "admin", "avatar_url": None,
                "exp": 0, "rank": None, "tier": None, "skills": []}
    return {**views.summary(dict(s)), "email": s["email"], "role": s["role"]}


@router.put("/me")
def update_me(body: dict, user=Depends(current_user)):
    _student_only(user)
    forge.fresh()
    with engine.connect() as c:
        s = dict(c.execute(select(students).where(students.c.id == user["id"])).mappings().first())
        mine = views.load_achievements(c, user["id"])
    clean = check_profile(body)
    before = {b["id"] for b in views.badges(s, mine)[0] if b["unlocked"]}
    with engine.begin() as c:
        c.execute(update(students).where(students.c.id == user["id"]).values(**clean, updated_at=utcnow()))
    forge.recompute()   # naam/skills badle -> search index bhi update
    after = views.badges({**s, **clean}, mine)[0]
    return {"unlocked": [b for b in after if b["unlocked"] and b["id"] not in before]}


@router.get("/dashboard")
def dashboard(user=Depends(current_user)):
    _student_only(user)
    forge.fresh()
    return views.dashboard(user["id"])


@router.get("/leaderboard")
def leaderboard(
    category: str = Query("all"),
    q: str = Query("", max_length=100),
    branch: str = "",
    batch: str = "",
    campus: str = "",
    tier: str = "",
    user=Depends(current_user),
):
    if category != "all" and category not in CATEGORY:
        raise HTTPException(404, "Unknown category.")
    forge.fresh()
    return views.leaderboard(category, q.strip(), branch, batch, campus, tier, user["id"])


@router.get("/students/{sid}")
def student(sid: int, user=Depends(optional_user)):
    forge.fresh()
    data = views.student_view(sid, user["id"] if user else None)
    if data is None:
        raise HTTPException(404, "This student does not exist.")
    return data
