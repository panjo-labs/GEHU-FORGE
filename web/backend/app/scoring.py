# Category weights aur platform multipliers = C++ Achievement.h jaise hi.
# EXP aur rank sirf C++ engine nikalta hai; yahan bas reviewer ko points preview dikhane ke liye.

CATEGORIES = [
    {"key": "competitive", "label": "Competitive programming", "short": "CP", "weight": 3.5, "cpp": "CODING"},
    {"key": "dsa", "label": "DSA", "short": "DSA", "weight": 3.0, "cpp": "DSA"},
    {"key": "project", "label": "Projects", "short": "Projects", "weight": 2.5, "cpp": "OSS"},
    {"key": "internship", "label": "Experience", "short": "Experience", "weight": 5.0, "cpp": "INTERN"},
    {"key": "certification", "label": "Learning", "short": "Learning", "weight": 1.0, "cpp": "CERT"},
    {"key": "academic", "label": "Academic", "short": "Academic", "weight": 1.5, "cpp": "ACAD"},
]
CATEGORY = {c["key"]: c for c in CATEGORIES}
PLATFORMS = ["Codeforces", "CodeChef", "LeetCode", "HackerRank", "Other"]
PLATFORM_MULTIPLIER = {"codeforces": 2.5, "codechef": 1.8, "leetcode": 1.2, "hackerrank": 0.8}

TIERS = [
    {"name": "Candidate", "min": 0},
    {"name": "Specialist", "min": 2000},
    {"name": "Expert", "min": 4500},
    {"name": "Master", "min": 7000},
    {"name": "Grandmaster", "min": 9000},
]
MAX_EXP = 10000


def tier_for(exp):
    current = TIERS[0]
    for t in TIERS:
        if exp >= t["min"]:
            current = t
    return current


def next_tier(exp):
    return next((t for t in TIERS if t["min"] > exp), None)


def weighted_points(category, score, platform=None):
    weight = CATEGORY[category]["weight"]
    mult = PLATFORM_MULTIPLIER.get((platform or "").lower(), 1.0) if category == "competitive" else 1.0
    return round(score * weight * mult, 2)


def top_percent(rank, total):
    if not rank or not total:
        return None
    return max(1, -(-rank * 100 // total))
