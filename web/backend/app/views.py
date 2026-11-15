# Pages ke liye ready JSON. Rank/EXP hamesha C++ engine ke cache se aata hai.
import math
from datetime import datetime, timedelta, timezone

from sqlalchemy import select

from .db import achievements, engine as db, students
from .engine import forge
from .scoring import CATEGORIES, CATEGORY, next_tier, tier_for, top_percent

IST = timezone(timedelta(hours=5, minutes=30))

BADGES = [
    ("first-verified", "First verified", "One achievement approved", "check", lambda c: c["approved"] >= 1),
    ("profile-complete", "Full profile", "Headline, bio, 3 skills and 2 links", "user", lambda c: c["profile_complete"]),
    ("all-rounder", "All-rounder", "Approved in 4 categories", "grid", lambda c: c["covered"] >= 4),
    ("industry-ready", "Industry ready", "An approved internship", "case", lambda c: c["by_cat"]["internship"] > 0),
    ("top-50", "Top 50 climber", "Reach #50 overall", "up", lambda c: bool(c["rank"]) and c["rank"] <= 50),
    ("top-10", "Top 10", "Reach #10 overall", "crown", lambda c: bool(c["rank"]) and c["rank"] <= 10),
    ("expert", "Expert tier", "Reach 4,500 EXP", "star", lambda c: c["exp"] >= 4500),
    ("streak-4", "Four-week streak", "Submit 4 weeks running", "flame", lambda c: c["streak"] >= 4),
]

PUBLIC_FIELDS = ("id", "roll_no", "name", "course", "branch", "section", "campus", "grad_year", "headline", "bio",
                 "avatar_url", "skills", "github", "leetcode", "codeforces", "codechef", "linkedin", "cf_rating",
                 "dsa_easy", "dsa_medium", "dsa_hard", "show_profile", "show_achievements", "show_rank", "role")


def iso_z(dt):
    return dt.strftime("%Y-%m-%dT%H:%M:%SZ") if dt else None


def ach_json(a):
    return {
        "id": a["id"], "student_id": a["student_id"], "category": a["category"], "title": a["title"],
        "organization": a["organization"], "description": a["description"],
        "achieved_on": a["achieved_on"].isoformat(), "score": a["score"], "platform": a["platform"],
        "proof_url": a["proof_url"], "status": a["status"], "points": a["points"], "review_note": a["review_note"],
        "submitted_at": iso_z(a["submitted_at"]), "reviewed_at": iso_z(a["reviewed_at"]),
    }


def load_students(conn):
    rows = conn.execute(select(students).where(students.c.role == "student")).mappings().all()
    return {r["id"]: dict(r) for r in rows}


def load_achievements(conn, student_id=None):
    q = select(achievements)
    if student_id is not None:
        q = q.where(achievements.c.student_id == student_id)
    return [dict(r) for r in conn.execute(q).mappings().all()]


def overall(sid):
    e = forge.get("all", sid)
    return (e[0], e[1]) if e else (0.0, None)


def tier_json(t):
    return {"name": t["name"], "min": t["min"]} if t else None


def summary(s):
    exp, rank = overall(s["id"])
    return {
        "id": s["id"], "name": s["name"], "branch": s["branch"], "grad_year": s["grad_year"], "campus": s["campus"],
        "roll_no": s["roll_no"], "headline": s["headline"], "skills": s["skills"] or [], "avatar_url": s["avatar_url"],
        "exp": exp, "rank": rank, "tier": tier_json(tier_for(exp)),
    }


def movement(category, sid):
    e = forge.get(category, sid)
    if not e:
        return None
    if e[3] is None:
        return {"kind": "new"}
    return {"kind": "rank", "value": e[3] - e[1]}


def week_streak(items, now):
    weeks = {math.floor((now - a["submitted_at"]).total_seconds() / (7 * 86400)) for a in items}
    w = 0 if 0 in weeks else 1
    streak = 0
    while w in weeks:
        streak += 1
        w += 1
    return streak


def profile_complete(s):
    links = sum(1 for k in ("github", "linkedin", "leetcode", "codeforces", "codechef") if s.get(k))
    return bool(s.get("headline") and s.get("bio") and len(s.get("skills") or []) >= 3 and links >= 2)


def badges(s, mine):
    approved = [a for a in mine if a["status"] == "approved"]
    by_cat = {c["key"]: sum(1 for a in approved if a["category"] == c["key"]) for c in CATEGORIES}
    exp, rank = overall(s["id"])
    ctx = {"approved": len(approved), "by_cat": by_cat, "covered": sum(1 for v in by_cat.values() if v),
           "rank": rank, "exp": exp, "streak": week_streak(mine, datetime.utcnow()), "profile_complete": profile_complete(s)}
    return [{"id": i, "name": n, "desc": d, "glyph": g, "unlocked": bool(t(ctx))} for i, n, d, g, t in BADGES], ctx


def categories(sid):
    out = []
    for c in CATEGORIES:
        board = forge.board(c["key"])
        e = board.get(sid)
        count = len(board)
        prev_count = sum(1 for v in board.values() if v[3] is not None)
        top = top_percent(e[1] if e else None, count)
        prev_top = top_percent(e[3] if e else None, prev_count)
        gain = (prev_top - top) if (top and prev_top and prev_top > top) else ((100 - top) if top and not prev_top else 0)
        out.append({"key": c["key"], "label": c["label"], "short": c["short"], "weight": c["weight"],
                    "exp": e[0] if e else 0, "rank": e[1] if e else None, "count": count, "top": top, "gain": gain})
    return out


def landing():
    with db.connect() as c:
        st = load_students(c)
        rows = c.execute(select(achievements.c.status)).scalars().all()
    counts = {}
    for r in rows:
        counts[r] = counts.get(r, 0) + 1
    ranked = sorted(forge.board("all").items(), key=lambda kv: kv[1][1])[:8]
    top = []
    for sid, e in ranked:
        s = st.get(sid)
        if not s:
            continue
        hidden = not s["show_rank"] or not s["show_profile"]
        row = summary(s)
        row.update(hidden=hidden, move=movement("all", sid))
        if hidden:
            row.update(name="Private student", avatar_url=None)
        top.append(row)
    return {"students": len(st), "verified": counts.get("approved", 0), "pending": counts.get("pending", 0),
            "ranked": forge.counts.get("all", 0), "top": top}


def dashboard(sid):
    with db.connect() as c:
        st = load_students(c)
        mine = load_achievements(c, sid)
    s = st[sid]
    now = datetime.utcnow()
    e = forge.get("all", sid)
    exp, rank = (e[0], e[1]) if e else (0.0, None)
    ranking = sorted(forge.board("all").items(), key=lambda kv: kv[1][1])

    def row_for(item):
        oid, v = item
        o = st[oid]
        hidden = oid != sid and not o["show_rank"]
        r = summary(o)
        r.update(name="Private student" if hidden else o["name"], hidden=hidden, exp=v[0], rank=v[1])
        return r

    top = [row_for(x) for x in ranking[:3] if x[0] in st]
    around = [row_for(x) for x in ranking[max(3, rank - 3): rank + 2] if x[0] in st] if rank else []
    board = {"top": top, "around": around, "gap": bool(rank) and max(4, rank - 2) > 4}

    tier, nt = tier_for(exp), next_tier(exp)
    cats = categories(sid)
    milestones = []
    if rank and rank > 1:
        aid, above = ranking[rank - 2]
        a = st.get(aid)
        gap = max(1, math.ceil(above[0] - exp))
        name = a["name"] if a and a["show_rank"] else "the student above"
        milestones.append({"kind": "pass", "title": f"Pass {name} for #{rank - 1}", "detail": f"{gap:,} EXP to go",
                           "progress": max(0.05, 1 - gap / 600)})
    if not rank:
        milestones.append({"kind": "start", "title": "Get your first achievement approved",
                           "detail": "You join the leaderboard as soon as one is verified", "progress": 0})
    if nt:
        milestones.append({"kind": "tier", "title": f"Reach {nt['name']}", "detail": f"{math.ceil(nt['min'] - exp):,} EXP to go",
                           "progress": (exp - tier["min"]) / (nt["min"] - tier["min"])})
    heavy = sorted([x for x in cats if x["weight"] >= 2.5], key=lambda x: x["top"] or 101)
    if heavy:
        w = heavy[-1]
        milestones.append({
            "kind": "category", "category": w["key"],
            "title": f"Lift {w['label']}" if w["top"] else f"Start {w['label']}",
            "detail": f"You are in the top {w['top']}%. Each entry counts ×{w['weight']}" if w["top"] else f"Each approved entry counts ×{w['weight']}",
            "progress": (100 - w["top"]) / 100 if w["top"] else 0,
        })

    today = datetime.now(IST).date()
    per_day = {}
    for a in mine:
        d = a["submitted_at"].replace(tzinfo=timezone.utc).astimezone(IST).date()
        per_day[d] = per_day.get(d, 0) + 1
    activity = [{"date": (today - timedelta(days=i)).isoformat(), "count": per_day.get(today - timedelta(days=i), 0)}
                for i in range(83, -1, -1)]

    mine.sort(key=lambda a: a["submitted_at"], reverse=True)
    badge_list, ctx = badges(s, mine)
    return {
        "student": {**summary(s), "email": s["email"]},
        "rank": rank, "ranked": forge.counts.get("all", 0), "totalStudents": len(st),
        "exp": exp, "expWeekAgo": e[2] if e else 0, "move": movement("all", sid),
        "tier": tier_json(tier), "nextTier": tier_json(nt), "top": top_percent(rank, forge.counts.get("all", 0)),
        "categories": cats, "milestones": milestones[:3], "board": board, "activity": activity,
        "weekStreak": ctx["streak"], "pending": sum(1 for a in mine if a["status"] == "pending"),
        "recent": [ach_json(a) for a in mine[:5]], "badges": badge_list,
    }


def leaderboard(category, q, branch, batch, campus, tier, viewer_id):
    board = forge.board(category)
    with db.connect() as c:
        st = load_students(c)
    filtered = bool(q or branch or batch or campus or tier)
    relevance = {sid: rel for sid, rel in forge.search(q)} if q else None

    rows = []
    for sid, s in st.items():
        me = sid == viewer_id
        if not me and not s["show_profile"]:
            continue
        e = board.get(sid)
        show_rank = me or s["show_rank"]
        if not filtered and (not e or not show_rank):
            continue
        if relevance is not None and sid not in relevance:
            continue
        base = summary(s)
        if branch and s["branch"] != branch:
            continue
        if batch and str(s["grad_year"]) != str(batch):
            continue
        if campus and s["campus"] != campus:
            continue
        if tier and base["tier"]["name"] != tier:
            continue
        base.update(self=me, catExp=e[0] if e else 0, catRank=(e[1] if e else None) if show_rank else None,
                    move=movement(category, sid) if show_rank else None,
                    relevance=relevance.get(sid) if relevance else None)
        rows.append(base)

    if relevance is not None:
        # Search: C++ engine ka relevance order (Trie + heap), phir rank
        rows.sort(key=lambda r: (-r["relevance"], r["catRank"] or 10**9, r["name"]))
    else:
        rows.sort(key=lambda r: (r["catRank"] or 10**9, r["name"]))

    me = None
    if viewer_id in st:
        e = board.get(viewer_id)
        me = {**summary(st[viewer_id]), "self": True, "catExp": e[0] if e else 0, "catRank": e[1] if e else None,
              "move": movement(category, viewer_id)}
    return {"category": category, "rows": rows, "total": len(board), "me": me, "filtered": filtered,
            "searched": relevance is not None}


def student_view(sid, viewer_id):
    with db.connect() as c:
        s = c.execute(select(students).where(students.c.id == sid, students.c.role == "student")).mappings().first()
        if s is None:
            return None
        s = dict(s)
        mine = load_achievements(c, sid)
    me = sid == viewer_id
    if not me and not s["show_profile"]:
        return {"private": True, "id": s["id"], "name": s["name"]}
    mine.sort(key=lambda a: a["achieved_on"], reverse=True)
    visible = mine if me else ([a for a in mine if a["status"] == "approved"] if s["show_achievements"] else [])
    show_rank = me or s["show_rank"]
    summ = summary(s)
    student = {k: s[k] for k in PUBLIC_FIELDS}
    student.update(exp=summ["exp"] if show_rank else None, rank=summ["rank"] if show_rank else None,
                   tier=summ["tier"] if show_rank else None, email=s["email"] if me else None)
    badge_list, _ = badges(s, mine)
    return {
        "self": me, "student": student, "ranked": forge.counts.get("all", 0),
        "move": movement("all", sid) if show_rank else None,
        "nextTier": tier_json(next_tier(summ["exp"])) if show_rank else None,
        "categories": categories(sid) if show_rank else [],
        "achievements": [ach_json(a) for a in visible],
        "achievementsHidden": not me and not s["show_achievements"],
        "badges": badge_list,
    }


def category_meta():
    return CATEGORY
