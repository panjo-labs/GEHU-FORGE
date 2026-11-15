# Reviewer ke liye auto-checks. Sirf free public APIs: Codeforces (bina key), GitHub (bina token 60/hour).
# Student ke diye link ko kabhi seedha fetch nahi karte (SSRF risk) - sirf pattern match karke in APIs ko call.
import re
from urllib.parse import urlparse

import httpx

from .config import GITHUB_TOKEN

TIMEOUT = httpx.Timeout(6.0)
PLACEHOLDER_HOSTS = {"example.com", "example.org", "example.edu", "localhost"}
KNOWN_HOSTS = {
    "codeforces.com": "Codeforces", "www.codechef.com": "CodeChef", "codechef.com": "CodeChef",
    "leetcode.com": "LeetCode", "www.hackerrank.com": "HackerRank", "github.com": "GitHub",
    "nptel.ac.in": "NPTEL", "archive.nptel.ac.in": "NPTEL", "www.coursera.org": "Coursera", "coursera.org": "Coursera",
    "www.credly.com": "Credly", "www.linkedin.com": "LinkedIn", "drive.google.com": "Google Drive",
}


def _check(level, text):
    return {"level": level, "text": text}


def _codeforces(url, handle):
    m = re.search(r"codeforces\.com/(?:contest|gym)/(\d+)", url)
    if not handle:
        return [_check("warn", "Student has no Codeforces handle on their profile, so the contest cannot be matched.")]
    try:
        r = httpx.get("https://codeforces.com/api/user.rating", params={"handle": handle}, timeout=TIMEOUT)
        data = r.json()
    except (httpx.HTTPError, ValueError):
        return [_check("info", "Codeforces could not be reached. Check the proof by hand.")]
    if data.get("status") != "OK":
        return [_check("warn", f"Codeforces has no user named '{handle}'.")]
    contests = data.get("result", [])
    out = [_check("ok", f"Codeforces user '{handle}' exists with {len(contests)} rated contests.")]
    if m:
        cid = int(m.group(1))
        hit = next((c for c in contests if c.get("contestId") == cid), None)
        if hit:
            out.append(_check("ok", f"Rank {hit['rank']} in '{hit['contestName']}' (rating {hit['oldRating']} → {hit['newRating']})."))
        else:
            out.append(_check("warn", f"'{handle}' has no rated result in contest {cid}."))
    return out


def _github(url, handle):
    headers = {"Accept": "application/vnd.github+json"}
    if GITHUB_TOKEN:
        headers["Authorization"] = f"Bearer {GITHUB_TOKEN}"
    pr = re.search(r"github\.com/([\w.-]+)/([\w.-]+)/pull/(\d+)", url)
    repo = re.search(r"github\.com/([\w.-]+)/([\w.-]+)/?$", url.split("?")[0])
    try:
        if pr:
            o, n, num = pr.groups()
            r = httpx.get(f"https://api.github.com/repos/{o}/{n}/pulls/{num}", headers=headers, timeout=TIMEOUT)
            if r.status_code == 404:
                return [_check("warn", "This pull request does not exist on GitHub.")]
            if r.status_code != 200:
                return [_check("info", "GitHub limit reached or unavailable. Check the proof by hand.")]
            d = r.json()
            author = d.get("user", {}).get("login", "")
            out = [_check("ok" if d.get("merged_at") else "warn",
                          "Pull request is merged." if d.get("merged_at") else f"Pull request is {d.get('state')} and not merged.")]
            if handle:
                same = author.lower() == handle.lower()
                out.append(_check("ok" if same else "warn",
                                  f"Opened by '{author}'" + ("" if same else f", not the student's handle '{handle}'.")))
            return out
        if repo:
            o, n = repo.groups()
            r = httpx.get(f"https://api.github.com/repos/{o}/{n}", headers=headers, timeout=TIMEOUT)
            if r.status_code == 404:
                return [_check("warn", "This repository does not exist or is private.")]
            if r.status_code != 200:
                return [_check("info", "GitHub limit reached or unavailable. Check the proof by hand.")]
            d = r.json()
            out = [_check("ok", f"Repository exists: {d.get('stargazers_count', 0)} stars, last push {str(d.get('pushed_at', ''))[:10]}.")]
            if handle:
                same = o.lower() == handle.lower()
                out.append(_check("ok" if same else "info", "Owned by the student." if same else f"Owned by '{o}', not '{handle}'. Check they contributed."))
            return out
    except (httpx.HTTPError, ValueError):
        return [_check("info", "GitHub could not be reached. Check the proof by hand.")]
    return [_check("info", "GitHub link is not a repository or pull request.")]


def run_checks(achievement, student):
    url = achievement["proof_url"]
    host = (urlparse(url).hostname or "").lower()
    results = []
    if host in PLACEHOLDER_HOSTS or host.endswith(".example.com"):
        return [_check("warn", f"Proof points to {host}, which is a placeholder, not a real record.")]
    results.append(_check("info", f"Proof hosted on {KNOWN_HOSTS.get(host, host)}."))
    if host == "codeforces.com":
        results += _codeforces(url, student.get("codeforces"))
    elif host == "github.com":
        results += _github(url, student.get("github"))
    elif achievement["category"] == "competitive" and (achievement.get("platform") or "").lower() == "codeforces":
        results.append(_check("warn", "Platform is Codeforces but the proof is not a codeforces.com link."))
    if host not in KNOWN_HOSTS:
        results.append(_check("info", "Unknown site. Open the link and check the name and date match."))
    return results
