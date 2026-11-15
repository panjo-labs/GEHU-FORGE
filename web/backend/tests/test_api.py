# Pura flow test: signup -> submit -> reviewer approve -> rank update -> search. Temp SQLite + asli C++ engine.
import os
import tempfile
from pathlib import Path

TMP = tempfile.mkdtemp()
os.environ["DATABASE_URL"] = f"sqlite:///{Path(TMP, 'test.db').as_posix()}"
os.environ["JWT_SECRET"] = "test-secret"
os.environ["ADMIN_PASSWORD"] = "reviewer-pass-123"
os.environ["AUTO_CHECKS"] = "false"

import pytest  # noqa: E402
from fastapi.testclient import TestClient  # noqa: E402

from app.main import app  # noqa: E402


@pytest.fixture(scope="module")
def client():
    with TestClient(app) as c:
        yield c


def auth(token):
    return {"Authorization": f"Bearer {token}"}


def signup(client, email="new.student@gehu.ac.in", roll="2612510999"):
    r = client.post("/api/auth/signup", json={"name": "New Student", "email": email, "password": "password123",
                                              "roll_no": roll, "branch": "CSE", "grad_year": 2030, "campus": "Dehradun"})
    return r


def reviewer(client):
    r = client.post("/api/auth/login", json={"email": "reviewer@gehu.ac.in", "password": "reviewer-pass-123"})
    assert r.status_code == 200
    return r.json()["token"]


def test_health_and_landing(client):
    h = client.get("/api/health").json()
    assert h["ok"] and h["engine"] and h["ranked"] > 200
    land = client.get("/api/landing").json()
    assert land["students"] == 250 and len(land["top"]) == 8
    assert land["top"][0]["rank"] == 1


def test_signup_rules(client):
    bad = client.post("/api/auth/signup", json={"name": "x", "email": "nope", "password": "123", "roll_no": "24BTCSE001"})
    assert bad.status_code == 422
    errs = bad.json()["error"]["errors"]
    assert {"name", "email", "password", "roll_no", "branch", "grad_year"} <= set(errs)
    assert "10-digit" in errs["roll_no"]


def test_full_review_flow(client):
    r = signup(client)
    assert r.status_code == 201
    token = r.json()["token"]
    assert client.get("/api/dashboard", headers=auth(token)).json()["rank"] is None
    assert signup(client).status_code == 422                      # same email dobara nahi

    bad = client.post("/api/achievements", headers=auth(token), json={"category": "competitive", "title": "Round",
                      "achieved_on": "2026-11-01", "score": 300, "proof_url": "https://codeforces.com/contest/1"})
    assert bad.status_code == 422 and "platform" in bad.json()["error"]["errors"]

    ok = client.post("/api/achievements", headers=auth(token), json={
        "category": "internship", "title": "Backend intern, 8 weeks", "organization": "Nimbus Labs",
        "achieved_on": "2026-10-20", "score": 600, "proof_url": "https://drive.google.com/file/d/abc"})
    assert ok.status_code == 201 and ok.json()["status"] == "pending"
    ach_id = ok.json()["id"]

    # Student reviewer route nahi khol sakta
    assert client.get("/api/admin/pending", headers=auth(token)).status_code == 403

    admin = reviewer(client)
    queue = client.get("/api/admin/pending", headers=auth(admin)).json()
    assert queue["total"] >= 100
    times = [i["submitted_at"] for i in queue["items"]]
    assert times == sorted(times)                                  # FIFO order

    assert client.post(f"/api/admin/review/{ach_id}", headers=auth(admin), json={"decision": "reject"}).status_code == 422
    res = client.post(f"/api/admin/review/{ach_id}", headers=auth(admin), json={"decision": "approve", "score": 550})
    assert res.status_code == 200
    body = res.json()
    assert body["status"] == "approved" and body["points"] == 550 * 5.0
    assert body["exp_after"] > 0 and body["rank_after"] is not None

    # Dobara review = 409 (state machine final hai)
    again = client.post(f"/api/admin/review/{ach_id}", headers=auth(admin), json={"decision": "reject", "note": "x"})
    assert again.status_code == 409

    dash = client.get("/api/dashboard", headers=auth(token)).json()
    assert dash["rank"] == body["rank_after"] and abs(dash["exp"] - body["exp_after"]) < 1e-6

    # Approved ab edit/delete nahi hoga
    assert client.delete(f"/api/achievements/{ach_id}", headers=auth(token)).status_code == 409


def test_search_uses_engine(client):
    token = client.post("/api/auth/demo", json={}).json()["token"]
    board = client.get("/api/leaderboard", headers=auth(token)).json()
    first = board["rows"][3]
    res = client.get("/api/leaderboard", params={"q": first["name"]}, headers=auth(token)).json()
    assert res["searched"] and any(r["id"] == first["id"] for r in res["rows"][:5])
    roll = client.get("/api/leaderboard", params={"q": first["roll_no"]}, headers=auth(token)).json()
    assert roll["rows"][0]["id"] == first["id"]
    typo = client.get("/api/leaderboard", params={"q": "pyhton"}, headers=auth(token)).json()
    assert len(typo["rows"]) > 0                                   # edit distance se python mila
    cat = client.get("/api/leaderboard", params={"category": "dsa"}, headers=auth(token)).json()
    ranks = [r["catRank"] for r in cat["rows"]]
    assert ranks == sorted(ranks) and ranks[0] == 1


def test_profile_update_and_badge(client):
    token = client.post("/api/auth/login", json={"email": "new.student@gehu.ac.in", "password": "password123"}).json()["token"]
    bad = client.put("/api/me", headers=auth(token), json={"name": "N", "linkedin": "linkedin.com/x"})
    assert bad.status_code == 422 and {"name", "linkedin"} <= set(bad.json()["error"]["errors"])
    good = client.put("/api/me", headers=auth(token), json={
        "name": "New Student", "headline": "Backend", "bio": "Learning", "skills": ["Rust", "Go", "SQL"],
        "github": "newstudent", "linkedin": "https://www.linkedin.com/in/new", "dsa_easy": 1, "dsa_medium": 0,
        "dsa_hard": 0, "show_profile": True, "show_achievements": True, "show_rank": True})
    assert good.status_code == 200
    assert any(b["id"] == "profile-complete" for b in good.json()["unlocked"])
    found = client.get("/api/leaderboard", params={"q": "rust"}, headers=auth(token)).json()["rows"]
    assert any(r["self"] for r in found)                           # search index update hua


def test_wrong_password_and_token(client):
    r = client.post("/api/auth/login", json={"email": "new.student@gehu.ac.in", "password": "wrong-pass"})
    assert r.status_code == 422
    assert client.get("/api/dashboard", headers=auth("garbage")).status_code == 401
    assert client.get("/api/dashboard").status_code == 401


def test_daily_visit_counts_for_consistency(client):
    tok = signup(client, email="streak.student@gehu.ac.in", roll="2612510998").json()["token"]
    d = client.get("/api/dashboard", headers=auth(tok)).json()
    c = d["consistency"]
    assert c["loginStreak"] == 1 and c["activeDays30"] == 1
    assert 1.0 < c["multiplier"] <= 1.25


def test_engine_restart_reloads_data(client):
    from app.engine import forge
    forge.proc.kill()
    forge.proc.wait()
    # Restart ke baad bhi search chalna chahiye (LOAD dobara bheja jata hai)
    r = client.get("/api/leaderboard?q=python", headers=auth(reviewer(client)))
    assert r.status_code == 200 and r.json()["rows"]
