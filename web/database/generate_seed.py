# GEHU Forge ka synthetic data (saare log fictional hain). Same seed = same data.
# Purane 100 students rakhta hai, 150 naye jodta hai, aur likhta hai:
#   data/*.csv                 -> C++ CLI ka format
#   web/backend/seed/forge.json -> backend ise database mein load karta hai
# Chalao (repo root se): python web/database/generate_seed.py
import csv
import json
import random
from datetime import date, datetime, time, timedelta, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "data"
OUT = Path(__file__).resolve().parent
SEED = 2026
TODAY = date(2026, 11, 10)
BASE_LAST_STUDENT = 2100
BASE_LAST_ACHIEVEMENT = 192
NEW_STUDENTS = 150

FIRST = ["Aarav", "Aditi", "Akash", "Anjali", "Ansh", "Arnav", "Bhavya", "Chirag", "Daksh", "Deepika",
         "Divya", "Gaurav", "Harsh", "Himani", "Isha", "Jatin", "Kavya", "Kriti", "Lakshya", "Manav",
         "Mansi", "Mohit", "Nandini", "Naveen", "Pallavi", "Parth", "Prachi", "Pranav", "Radhika", "Rahul",
         "Ritika", "Sahil", "Sakshi", "Shivam", "Shreya", "Siddharth", "Simran", "Tanmay", "Tanvi", "Uday",
         "Vaibhav", "Vanshika", "Varun", "Yashika", "Zoya"]
LAST = ["Agarwal", "Bisht", "Chauhan", "Dhoundiyal", "Dobhal", "Gairola", "Joshi", "Kandpal", "Kumar",
        "Malhotra", "Mishra", "Negi", "Pandey", "Pant", "Rawat", "Semwal", "Sharma", "Singh", "Thapliyal",
        "Tiwari", "Uniyal", "Verma"]
BRANCHES = ["CSE", "CSE", "CSE", "IT", "ECE", "AIML", "DS"]
SKILLS = ["C++", "DSA", "Python", "Java", "Web", "SQL", "Cloud", "DevOps", "ML", "Git", "Linux",
          "React", "Embedded", "Security"]
PLATFORMS = ["Codeforces", "CodeChef", "LeetCode", "HackerRank"]
TITLES = {
    "CODING": ["Div2 Contest Top 500", "Weekly Contest Top 100", "Rated Round Finish", "Starters Rank 50",
               "Practice Milestone"],
    "CERT": ["Cloud Fundamentals Cert", "Data Structures Course", "Networking Cert", "Python Specialisation",
             "DBMS Certificate"],
    "OSS": ["Merged Pull Request", "Bug Fix Contribution", "Docs Improvement", "Feature Contribution",
            "Issue Triage"],
    "INTERN": ["Software Intern", "Data Intern", "Web Dev Intern", "Cloud Intern", "Research Intern"],
}
CATEGORY = {"CODING": "competitive", "CERT": "certification", "OSS": "project", "INTERN": "internship"}
DSA_TITLES = ["100 problems solved", "250 problems solved", "Dynamic programming set completed",
              "Graphs set completed", "50-day daily challenge streak", "Trees and recursion set completed"]
ACADEMIC_TITLES = ["Inter-college hackathon, 2nd place", "Dean's list, semester 4", "Technical paper presented",
                   "Coding club core member", "Robotics competition finalist", "Department topper, semester 3"]
ORGS = {
    "competitive": None,
    "dsa": None,
    "project": ["GitHub", "Open source", "Personal project"],
    "internship": ["Nimbus Labs", "Pinecrest Analytics", "Doon Softworks", "Kestrel Cloud", "Himalaya Robotics",
                   "Larkspur Fintech"],
    "certification": ["NPTEL", "Coursera", "AWS Academy", "Cisco Networking Academy", "Google Cloud Skills Boost"],
    "academic": ["Graphic Era Hill University", "Department of CSE", "University coding club"],
}
DESCRIPTIONS = {
    "project": ["Built the API layer and wrote tests for the main flows.",
                "Fixed a pagination bug and added a regression test.",
                "Improved the setup guide so new contributors get running in minutes."],
    "internship": ["Worked on internal dashboards with React and a Python API.",
                   "Cleaned and modelled sales data; built weekly reports.",
                   "Helped migrate services to containers and set up CI."],
}
HEADLINES = ["Competitive programmer", "Backend and APIs", "Frontend developer", "ML and data", "Cloud and DevOps",
             "Embedded systems", "Full-stack developer", "Security enthusiast"]
REJECT_NOTES = ["Proof link does not show the result.", "Duplicate of an earlier submission.",
                "Certificate name does not match the student.", "Score claimed is higher than the proof shows."]
# Roll no. 10 digit: saal(2) + 1 + branch code(2) + 1 + serial(4), jaise 2512510044
BRANCH_CODE = {"CSE": "25", "IT": "26", "ECE": "31", "AIML": "27", "DS": "28"}
BIO = ["{b} student who enjoys {s1} and {s2}.", "Learning {s1}, building with {s2}.",
       "{b} batch of {y}. Into {s1}.", "Competitive programmer, also likes {s2}.", ""]


def read_csv(path):
    with open(path, newline="", encoding="utf8") as f:
        return list(csv.DictReader(f))


def parse_dmy(s):
    d, m, y = (int(x) for x in s.split("-"))
    return date(y, m, d)


def past(d):
    # Purane generator ki future dates ko ek saal peeche karo
    return d.replace(year=d.year - 1) if d > TODAY else d


def recent_date(rng, max_days=520):
    # 2/3 activity pichhle 4 mahine mein, real semester jaisa
    days = rng.randint(0, 120) if rng.random() < 0.66 else rng.randint(121, max_days)
    return TODAY - timedelta(days=days)


def make_students(rng):
    base = [r for r in read_csv(DATA / "students.csv") if int(r["id"]) <= BASE_LAST_STUDENT]
    rows = [dict(r) for r in base]
    used_emails = {r["email"] for r in rows}
    for sid in range(BASE_LAST_STUDENT + 1, BASE_LAST_STUDENT + 1 + NEW_STUDENTS):
        first, last = rng.choice(FIRST), rng.choice(LAST)
        handle = f"{first}_{last}{sid}"
        email = f"{first}.{last}{sid}@example.edu".lower()
        assert email not in used_emails
        used_emails.add(email)
        skills = list(dict.fromkeys(rng.sample(SKILLS, rng.randint(2, 5))))
        rows.append({"id": str(sid), "name": f"{first} {last}", "email": email,
                     "branch": rng.choice(BRANCHES), "gradYear": str(rng.choice([2027, 2027, 2028, 2028, 2029, 2030])),
                     "github": f"gh_{handle}", "leetcode": f"lc_{handle}", "skills": ";".join(skills)})
    return rows


def make_achievements(rng, students):
    rows = []
    for r in read_csv(DATA / "achievements.csv"):
        if int(r["id"]) > BASE_LAST_ACHIEVEMENT:
            continue
        r = dict(r)
        r["date"] = past(parse_dmy(r["date"])).strftime("%d-%m-%Y")
        rows.append(r)
    next_id = BASE_LAST_ACHIEVEMENT + 1
    for s in students:
        sid = int(s["id"])
        if sid <= BASE_LAST_STUDENT:
            continue
        for _ in range(rng.choice([0, 1, 1, 2, 2, 3, 3, 4, 5])):
            kind = rng.randrange(10)
            if kind < 4:
                t, score, platform = "CODING", 50 + rng.randrange(450), rng.choice(PLATFORMS)
            elif kind < 6:
                t, score, platform = "CERT", 50 + rng.randrange(250), ""
            elif kind < 9:
                t, score, platform = "OSS", 50 + rng.randrange(350), ""
            else:
                t, score, platform = "INTERN", 300 + rng.randrange(700), ""
            roll = rng.randrange(100)
            status = "Approved" if roll < 78 else ("Pending" if roll < 93 else "Rejected")
            d = TODAY - timedelta(days=rng.randint(0, 25)) if status == "Pending" else recent_date(rng)
            rows.append({"id": str(next_id), "studentId": str(sid), "type": t, "title": rng.choice(TITLES[t]),
                         "date": d.strftime("%d-%m-%Y"), "score": str(score), "platform": platform,
                         "proof": f"proof_{next_id}.pdf", "status": status})
            next_id += 1
    return rows


def enrich_students(rng, students, achievements):
    platforms_used = {}
    for a in achievements:
        if a["platform"]:
            platforms_used.setdefault(a["studentId"], set()).add(a["platform"])
    seq = {}
    out = []
    for s in students:
        sid = s["id"]
        grad = int(s["gradYear"])
        key = (grad - 4, s["branch"])
        seq[key] = seq.get(key, 0) + 1
        skills = list(dict.fromkeys(x for x in s["skills"].split(";") if x))
        s1, s2 = (skills + ["DSA", "Web"])[:2]
        handle = s["github"][3:]
        used = platforms_used.get(sid, set())
        coder = bool(used) or rng.random() < 0.5
        out.append({
            "id": sid,
            "roll_no": f"{(grad - 4) % 100:02d}1{BRANCH_CODE[s['branch']]}1{seq[key]:04d}",
            "name": s["name"],
            "email": s["email"],
            "role": "student",
            "course": "B.Tech",
            "branch": s["branch"],
            "section": rng.choice("ABCD"),
            "campus": rng.choices(["Dehradun", "Bhimtal", "Haldwani"], weights=[70, 15, 15])[0],
            "grad_year": grad,
            "headline": rng.choice(HEADLINES),
            "bio": rng.choice(BIO).format(b=s["branch"], s1=s1, s2=s2, y=grad),
            "skills": skills[:10],
            "github": s["github"],
            "leetcode": s["leetcode"],
            "codeforces": f"cf_{handle}" if "Codeforces" in used else "",
            "codechef": f"cc_{handle}" if "CodeChef" in used else "",
            "linkedin": f"https://www.linkedin.com/in/{handle.lower().replace('_', '-')}" if rng.random() < 0.7 else "",
            "cf_rating": rng.randint(800, 1900) if "Codeforces" in used else None,
            "dsa_easy": rng.randint(40, 220) if coder else rng.randint(0, 40),
            "dsa_medium": rng.randint(20, 260) if coder else rng.randint(0, 20),
            "dsa_hard": rng.randint(0, 60) if coder else rng.randint(0, 3),
            "show_profile": rng.random() > 0.03,
            "show_achievements": rng.random() > 0.05,
            "show_rank": rng.random() > 0.05,
            "created_at": datetime.combine(TODAY - timedelta(days=rng.randint(10, 90)), time(rng.randint(8, 22), rng.randint(0, 59)), timezone.utc),
        })
    return out


def enrich_achievements(rng, achievements):
    out = []
    for a in achievements:
        d = parse_dmy(a["date"])
        submitted = datetime.combine(min(d + timedelta(days=rng.randint(0, 3)), TODAY),
                                     time(rng.randint(8, 23), rng.randint(0, 59)), timezone.utc)
        status = a["status"].lower()
        reviewed = None
        if status != "pending":
            reviewed = min(submitted + timedelta(days=rng.randint(1, 5), hours=rng.randint(0, 8)),
                           datetime.combine(TODAY, time(9, 0), timezone.utc))
            reviewed = max(reviewed, submitted)
        cat = CATEGORY[a["type"]]
        out.append({
            "id": int(a["id"]),
            "student_id": int(a["studentId"]),
            "category": cat,
            "title": a["title"],
            "organization": rng.choice(ORGS[cat]) if ORGS[cat] else "",
            "description": rng.choice(DESCRIPTIONS[cat]) if cat in DESCRIPTIONS else "",
            "achieved_on": d,
            "score": int(a["score"]),
            "platform": a["platform"],
            "proof_url": f"https://example.com/proof/{a['id']}",
            "status": status,
            "review_note": rng.choice(REJECT_NOTES) if status == "rejected" else "",
            "submitted_at": submitted,
            "reviewed_at": reviewed,
        })
    return out


def extra_achievements(rng, db_students, next_id):
    # DSA/academic sirf database mein; data/*.csv purane C++ CLI format ke liye hai
    out = []
    for s in db_students:
        for cat, titles, chance, lo, hi in (("dsa", DSA_TITLES, 0.55, 60, 320), ("academic", ACADEMIC_TITLES, 0.3, 80, 400)):
            if rng.random() > chance:
                continue
            roll = rng.randrange(100)
            status = "approved" if roll < 80 else ("pending" if roll < 94 else "rejected")
            d = TODAY - timedelta(days=rng.randint(0, 25)) if status == "pending" else recent_date(rng)
            submitted = datetime.combine(min(d + timedelta(days=rng.randint(0, 3)), TODAY),
                                         time(rng.randint(8, 23), rng.randint(0, 59)), timezone.utc)
            reviewed = None
            if status != "pending":
                reviewed = max(submitted, min(submitted + timedelta(days=rng.randint(1, 5)),
                                              datetime.combine(TODAY, time(9, 0), timezone.utc)))
            out.append({
                "id": next_id, "student_id": s["id"] if isinstance(s["id"], int) else int(s["id"]),
                "category": cat, "title": rng.choice(titles),
                "organization": rng.choice(ORGS[cat]) if ORGS[cat] else "",
                "description": "",
                "achieved_on": d, "score": rng.randint(lo, hi),
                "platform": rng.choice(["LeetCode", "Codeforces", "CodeChef", "HackerRank"]) if cat == "dsa" else "",
                "proof_url": f"https://example.com/proof/{next_id}", "status": status,
                "review_note": rng.choice(REJECT_NOTES) if status == "rejected" else "",
                "submitted_at": submitted, "reviewed_at": reviewed,
            })
            next_id += 1
    return out


def to_json(v):
    if isinstance(v, datetime):
        return v.isoformat().replace("+00:00", "Z")
    if isinstance(v, date):
        return v.isoformat()
    return None if v == "" else v


def write_csv(path, fields, rows):
    with open(path, "w", newline="", encoding="utf8") as f:
        w = csv.DictWriter(f, fieldnames=fields, lineterminator="\n")
        w.writeheader()
        w.writerows(rows)


def main():
    rng = random.Random(SEED)
    students = make_students(rng)
    achievements = make_achievements(rng, students)

    write_csv(DATA / "students.csv", ["id", "name", "email", "branch", "gradYear", "github", "leetcode", "skills"], students)
    write_csv(DATA / "achievements.csv", ["id", "studentId", "type", "title", "date", "score", "platform", "proof", "status"], achievements)

    db_students = enrich_students(rng, students, achievements)
    db_achievements = enrich_achievements(rng, achievements)
    for s in db_students:
        s["id"] = int(s["id"])
    db_achievements += extra_achievements(rng, db_students, max(a["id"] for a in db_achievements) + 1)

    s_cols = ["id", "roll_no", "name", "email", "role", "course", "branch", "section", "campus", "grad_year",
              "headline", "bio", "skills", "github", "leetcode", "codeforces", "codechef", "linkedin", "cf_rating",
              "dsa_easy", "dsa_medium", "dsa_hard", "show_profile", "show_achievements", "show_rank", "created_at"]
    a_cols = ["id", "student_id", "category", "title", "organization", "description", "achieved_on", "score",
              "platform", "proof_url", "status", "review_note", "submitted_at", "reviewed_at"]

    out = ROOT / "web" / "backend" / "seed"
    out.mkdir(parents=True, exist_ok=True)
    with open(out / "forge.json", "w", encoding="utf8") as f:
        json.dump({"generated": TODAY.isoformat(),
                   "students": [{c: to_json(r[c]) for c in s_cols} for r in db_students],
                   "achievements": [{c: to_json(r[c]) for c in a_cols} for r in db_achievements]},
                  f, separators=(",", ":"))

    counts = {}
    for a in db_achievements:
        counts[a["status"]] = counts.get(a["status"], 0) + 1
    print(f"students: {len(db_students)}, achievements: {len(db_achievements)} {counts}")


if __name__ == "__main__":
    main()
