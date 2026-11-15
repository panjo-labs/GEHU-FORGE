import re
from datetime import date

from .scoring import CATEGORY, PLATFORMS

BRANCHES = ["CSE", "IT", "ECE", "AIML", "DS"]
CAMPUSES = ["Dehradun", "Bhimtal", "Haldwani"]
GRAD_YEARS = [2026, 2027, 2028, 2029, 2030]


class ValidationFailed(Exception):
    # errors = {field: message}, frontend har field ke neeche dikhata hai
    def __init__(self, errors):
        super().__init__("Some fields need attention.")
        self.errors = errors


def _int(v):
    try:
        if isinstance(v, bool):
            return None
        f = float(v)
        return int(f) if f.is_integer() else None
    except (TypeError, ValueError):
        return None


def clean_str(v, limit=None):
    s = (v or "").strip() if isinstance(v, str) else ""
    return s[:limit] if limit else s


def check_achievement(data):
    e = {}
    a = {
        "category": data.get("category"),
        "title": clean_str(data.get("title")),
        "organization": clean_str(data.get("organization")) or None,
        "description": clean_str(data.get("description")) or None,
        "platform": clean_str(data.get("platform")) or None,
        "proof_url": clean_str(data.get("proof_url")),
        "score": _int(data.get("score")),
        "achieved_on": None,
    }
    if a["category"] not in CATEGORY:
        e["category"] = "Choose a category."
    if len(a["title"]) < 3:
        e["title"] = "Add a title of at least 3 characters."
    elif len(a["title"]) > 120:
        e["title"] = "Keep the title under 120 characters."
    if a["organization"] and len(a["organization"]) > 80:
        e["organization"] = "Keep this under 80 characters."
    if a["description"] and len(a["description"]) > 300:
        e["description"] = "Keep the description under 300 characters."
    try:
        a["achieved_on"] = date.fromisoformat(str(data.get("achieved_on") or ""))
        if a["achieved_on"] > date.today():
            e["achieved_on"] = "The date cannot be in the future."
    except ValueError:
        e["achieved_on"] = "Pick the date."
    if a["score"] is None:
        e["score"] = "Enter whole points from 0 to 1000."
    elif not 0 <= a["score"] <= 1000:
        e["score"] = "Points must be between 0 and 1000."
    if a["category"] not in ("competitive", "dsa"):
        a["platform"] = None
    if a["category"] == "competitive" and not a["platform"]:
        e["platform"] = "Pick the contest platform."
    elif a["platform"] and a["platform"] not in PLATFORMS:
        e["platform"] = "Pick a platform from the list."
    if not re.match(r"^https://[^\s]+\.[^\s]+", a["proof_url"]) or len(a["proof_url"]) > 500:
        e["proof_url"] = "Proof must be a link starting with https://"
    if e:
        raise ValidationFailed(e)
    return a


def check_profile(data):
    e = {}
    p = {
        "name": clean_str(data.get("name")),
        "headline": clean_str(data.get("headline")) or None,
        "bio": clean_str(data.get("bio")) or None,
        "github": clean_str(data.get("github")) or None,
        "linkedin": clean_str(data.get("linkedin")) or None,
        "leetcode": clean_str(data.get("leetcode")) or None,
        "codeforces": clean_str(data.get("codeforces")) or None,
        "codechef": clean_str(data.get("codechef")) or None,
        "show_profile": bool(data.get("show_profile", True)),
        "show_achievements": bool(data.get("show_achievements", True)),
        "show_rank": bool(data.get("show_rank", True)),
        "avatar_url": data.get("avatar_url") or None,
    }
    skills = []
    for s in data.get("skills") or []:
        s = clean_str(s, 24)
        if s and s.lower() not in [x.lower() for x in skills]:
            skills.append(s)
    p["skills"] = skills
    if not 2 <= len(p["name"]) <= 80:
        e["name"] = "Name must be 2 to 80 characters."
    if p["headline"] and len(p["headline"]) > 80:
        e["headline"] = "Keep the headline under 80 characters."
    if p["bio"] and len(p["bio"]) > 200:
        e["bio"] = "Keep the bio under 200 characters."
    if len(skills) > 10:
        e["skills"] = "Up to 10 skills."
    for k in ("github", "leetcode", "codeforces", "codechef"):
        if p[k] and (re.search(r"\s", p[k]) or len(p[k]) > 60):
            e[k] = "Handles cannot contain spaces."
    if p["linkedin"] and not re.match(r"^https://(www\.)?linkedin\.com/.+", p["linkedin"], re.I):
        e["linkedin"] = "Use your full LinkedIn link (https://www.linkedin.com/in/…)."
    for k in ("dsa_easy", "dsa_medium", "dsa_hard"):
        v = _int(data.get(k, 0))
        if v is None or not 0 <= v <= 5000:
            e[k] = "Enter a number from 0 to 5000."
        p[k] = v or 0
    cf = data.get("cf_rating")
    if cf in (None, ""):
        p["cf_rating"] = None
    else:
        v = _int(cf)
        if v is None or not 0 <= v <= 4000:
            e["cf_rating"] = "Rating must be 0 to 4000."
        p["cf_rating"] = v
    av = p["avatar_url"]
    if av and (not isinstance(av, str) or not av.startswith("data:image/") or len(av) > 400_000):
        e["avatar_url"] = "Use a JPG, PNG or WebP photo under 300 KB."
    if e:
        raise ValidationFailed(e)
    return p


def check_signup(data, email_taken):
    e = {}
    f = {
        "name": clean_str(data.get("name")),
        "email": clean_str(data.get("email")).lower(),
        "password": data.get("password") or "",
        "roll_no": clean_str(data.get("roll_no")),
        "branch": data.get("branch"),
        "grad_year": _int(data.get("grad_year")),
        "campus": data.get("campus") or "Dehradun",
    }
    if len(f["name"]) < 2:
        e["name"] = "Enter your full name."
    if not re.match(r"^[^@\s]+@[^@\s]+\.[^@\s]+$", f["email"]):
        e["email"] = "Enter a valid email address."
    elif email_taken(f["email"]):
        e["email"] = "An account with this email already exists. Sign in instead."
    if len(f["password"]) < 8:
        e["password"] = "Use at least 8 characters."
    elif len(f["password"]) > 72:
        e["password"] = "Use at most 72 characters."
    if not re.fullmatch(r"\d{10}", f["roll_no"]):
        e["roll_no"] = "Enter your 10-digit university roll number."
    if f["branch"] not in BRANCHES:
        e["branch"] = "Choose your branch."
    if f["grad_year"] not in GRAD_YEARS:
        e["grad_year"] = "Choose your batch."
    if f["campus"] not in CAMPUSES:
        e["campus"] = "Choose your campus."
    if e:
        raise ValidationFailed(e)
    return f
