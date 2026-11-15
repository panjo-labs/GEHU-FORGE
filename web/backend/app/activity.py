# Student ne aaj platform khola? Din mein ek row (student_id, day). Isi se login streak banta hai.
import random
import threading
from datetime import datetime, timedelta, timezone

from sqlalchemy import func, insert, select

from .db import activity, engine as db, students

IST = timezone(timedelta(hours=5, minutes=30))
_seen = set()            # (sid, day) jo is process mein likh chuke - har request pe DB hit nahi
_lock = threading.Lock()


def today():
    return datetime.now(IST).date()


def mark_active(sid):
    """Aaj ka pehla visit ho toh True (rank dobara nikalna padega)."""
    key = (sid, today())
    with _lock:
        if key in _seen:
            return False
        _seen.add(key)
    with db.begin() as c:
        exists = c.execute(select(activity.c.student_id).where(
            activity.c.student_id == sid, activity.c.day == key[1])).first()
        if exists:
            return False
        c.execute(insert(activity).values(student_id=sid, day=key[1]))
    return True


def recent_days(c, days=60):
    """sid -> pichhle `days` din ki active dates (C++ engine ko bhejne ke liye)."""
    since = today() - timedelta(days=days)
    out = {}
    for sid, day in c.execute(select(activity.c.student_id, activity.c.day).where(activity.c.day >= since)):
        out.setdefault(sid, []).append(day.isoformat())
    return out


def seed_if_empty(conn):
    # Synthetic history sirf demo data ke liye: har student ki apni "aadat" (seed = id, har baar same)
    if conn.execute(select(func.count()).select_from(activity)).scalar():
        return 0
    end = today() - timedelta(days=1)
    rows = []
    for (sid,) in conn.execute(select(students.c.id).where(students.c.role == "student")):
        rng = random.Random(sid)
        habit = rng.choice([0.05, 0.15, 0.3, 0.5, 0.7, 0.9])
        streak = rng.choice([0, 0, 0, 2, 4, 7, 12, 20]) if habit >= 0.3 else 0
        for i in range(60):
            if i < streak or rng.random() < habit:
                rows.append({"student_id": sid, "day": end - timedelta(days=i)})
    if rows:
        conn.execute(insert(activity), rows)
    return len(rows)
