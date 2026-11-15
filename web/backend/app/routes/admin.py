from datetime import timedelta

from fastapi import APIRouter, Body, Depends, HTTPException, Query
from sqlalchemy import func, select, update

from .. import views
from ..db import achievements, engine, students, utcnow
from ..engine import forge
from ..scoring import CATEGORY, weighted_points
from ..security import require_admin
from .achievements import store_checks

router = APIRouter(prefix="/admin", tags=["Reviewer"])
PAGE = 20


@router.get("/stats")
def stats(admin=Depends(require_admin)):
    since = utcnow() - timedelta(hours=24)
    with engine.connect() as c:
        pending = c.execute(select(func.count()).select_from(achievements).where(achievements.c.status == "pending")).scalar()
        today = c.execute(select(achievements.c.status, func.count()).where(achievements.c.reviewed_at >= since)
                          .group_by(achievements.c.status)).all()
        oldest = c.execute(select(func.min(achievements.c.submitted_at)).where(achievements.c.status == "pending")).scalar()
    done = dict(today)
    return {"pending": pending, "approved24h": done.get("approved", 0), "rejected24h": done.get("rejected", 0),
            "oldest": views.iso_z(oldest)}


@router.get("/pending")
def pending(page: int = Query(1, ge=1), category: str = "", admin=Depends(require_admin)):
    # FIFO: sabse purana submission sabse pehle (C++ LinkedQueue jaisa)
    q = select(achievements).where(achievements.c.status == "pending")
    if category:
        if category not in CATEGORY:
            raise HTTPException(404, "Unknown category.")
        q = q.where(achievements.c.category == category)
    with engine.connect() as c:
        total = c.execute(select(func.count()).select_from(q.subquery())).scalar()
        rows = c.execute(q.order_by(achievements.c.submitted_at, achievements.c.id).offset((page - 1) * PAGE).limit(PAGE)).mappings().all()
        ids = {r["student_id"] for r in rows}
        st = {s["id"]: dict(s) for s in c.execute(select(students).where(students.c.id.in_(ids))).mappings()} if ids else {}
    forge.fresh()
    items = []
    for r in rows:
        s = st.get(r["student_id"])
        summ = views.summary(s) if s else None
        items.append({
            **views.ach_json(r),
            "points_preview": weighted_points(r["category"], r["score"], r["platform"]),
            "auto_check": r["auto_check"],
            "student": summ and {**summ, "github": s["github"], "codeforces": s["codeforces"], "leetcode": s["leetcode"]},
        })
    return {"page": page, "pageSize": PAGE, "total": total, "items": items}


@router.post("/recheck/{ach_id}")
def recheck(ach_id: int, admin=Depends(require_admin)):
    with engine.connect() as c:
        if c.execute(select(achievements.c.id).where(achievements.c.id == ach_id)).first() is None:
            raise HTTPException(404, "This achievement was not found.")
    store_checks(ach_id)
    with engine.connect() as c:
        return {"auto_check": c.execute(select(achievements.c.auto_check).where(achievements.c.id == ach_id)).scalar()}


@router.post("/review/{ach_id}")
def review(ach_id: int, body: dict = Body(...), admin=Depends(require_admin)):
    decision = str(body.get("decision", "")).strip().lower()
    note = (str(body.get("note") or "").strip() or None)
    if decision not in ("approve", "reject"):
        raise HTTPException(422, "Decision must be approve or reject.")
    if note and len(note) > 300:
        raise HTTPException(422, "Keep the note under 300 characters.")
    if decision == "reject" and not note:
        raise HTTPException(422, "Add a short note so the student knows why it was rejected.")

    with engine.connect() as c:
        a = c.execute(select(achievements).where(achievements.c.id == ach_id)).mappings().first()
    if a is None:
        raise HTTPException(404, "This achievement was not found.")
    score = a["score"]
    if decision == "approve" and body.get("score") not in (None, ""):
        try:
            score = int(body["score"])
        except (TypeError, ValueError):
            raise HTTPException(422, "Points must be a whole number from 0 to 1000.")
        if not 0 <= score <= 1000:
            raise HTTPException(422, "Points must be a whole number from 0 to 1000.")

    before = forge.get("all", a["student_id"])
    values = {"reviewed_at": utcnow(), "review_note": note, "reviewed_by": admin["id"]}
    if decision == "approve":
        values.update(status="approved", score=score, points=weighted_points(a["category"], score, a["platform"]), points_applied=True)
    else:
        values.update(status="rejected", points=None, points_applied=False)

    # Conditional update: do reviewers ek saath dabayein toh sirf ek jeetega
    with engine.begin() as c:
        res = c.execute(update(achievements).where(achievements.c.id == ach_id, achievements.c.status == "pending").values(**values))
        if res.rowcount == 0:
            raise HTTPException(409, "Someone already reviewed this achievement.")

    # EXP approved rows se dobara banta hai, isliye ek achievement do baar count ho hi nahi sakta
    if decision == "approve":
        forge.recompute()
    after = forge.get("all", a["student_id"])
    return {
        "id": ach_id, "status": values["status"], "points": values["points"],
        "exp_before": before[0] if before else 0, "exp_after": after[0] if after else 0,
        "rank_before": before[1] if before else None, "rank_after": after[1] if after else None,
    }

