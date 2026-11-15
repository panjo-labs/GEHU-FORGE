from fastapi import APIRouter, BackgroundTasks, Depends, HTTPException
from sqlalchemy import delete, insert, select, update

from ..config import AUTO_CHECKS
from ..db import achievements, engine, students, utcnow
from ..security import current_user
from ..validation import check_achievement
from ..verify import run_checks
from ..views import ach_json

router = APIRouter(prefix="/achievements", tags=["Achievements"])


def store_checks(ach_id):
    # Background mein free APIs se checks, reviewer ko queue mein dikhte hain
    with engine.connect() as c:
        a = c.execute(select(achievements).where(achievements.c.id == ach_id)).mappings().first()
        if a is None:
            return
        s = c.execute(select(students).where(students.c.id == a["student_id"])).mappings().first()
    result = run_checks(dict(a), dict(s))
    with engine.begin() as c:
        c.execute(update(achievements).where(achievements.c.id == ach_id).values(auto_check=result))


def _student(user):
    if user["role"] != "student":
        raise HTTPException(403, "Reviewer accounts cannot submit achievements.")


def _owned_pending(c, user_id, ach_id):
    a = c.execute(select(achievements).where(achievements.c.id == ach_id)).mappings().first()
    if a is None or a["student_id"] != user_id:
        raise HTTPException(404, "This achievement was not found.")
    return a


@router.post("", status_code=201)
def submit(body: dict, tasks: BackgroundTasks, user=Depends(current_user)):
    _student(user)
    a = check_achievement(body)
    with engine.begin() as c:
        new_id = c.execute(insert(achievements).values(
            **a, student_id=user["id"], status="pending", points_applied=False, submitted_at=utcnow(),
        )).inserted_primary_key[0]
        row = c.execute(select(achievements).where(achievements.c.id == new_id)).mappings().first()
    if AUTO_CHECKS:
        tasks.add_task(store_checks, new_id)
    return ach_json(row)


@router.get("/mine")
def mine(user=Depends(current_user)):
    with engine.connect() as c:
        rows = c.execute(select(achievements).where(achievements.c.student_id == user["id"])
                         .order_by(achievements.c.submitted_at.desc())).mappings().all()
    return {"items": [ach_json(r) for r in rows]}


@router.put("/{ach_id}")
def edit(ach_id: int, body: dict, tasks: BackgroundTasks, user=Depends(current_user)):
    _student(user)
    a = check_achievement(body)
    with engine.begin() as c:
        _owned_pending(c, user["id"], ach_id)
        # Sirf pending badal sakta hai - WHERE status='pending' race se bhi bachata hai
        res = c.execute(update(achievements)
                        .where(achievements.c.id == ach_id, achievements.c.status == "pending")
                        .values(**a, auto_check=None))
        if res.rowcount == 0:
            raise HTTPException(409, "This achievement was already reviewed and cannot be changed.")
    if AUTO_CHECKS:
        tasks.add_task(store_checks, ach_id)
    return {"ok": True}


@router.delete("/{ach_id}")
def remove(ach_id: int, user=Depends(current_user)):
    _student(user)
    with engine.begin() as c:
        _owned_pending(c, user["id"], ach_id)
        res = c.execute(delete(achievements).where(achievements.c.id == ach_id, achievements.c.status != "approved"))
        if res.rowcount == 0:
            raise HTTPException(409, "Approved achievements stay on your record.")
    return {"ok": True}
