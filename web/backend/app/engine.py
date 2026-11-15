import logging
import shutil
import subprocess
import threading
from datetime import datetime, timedelta

from sqlalchemy import delete, insert, select

from .config import CACHE_DIR, ENGINE_PATH, ENGINE_SOURCE
from .db import achievements, engine as db, scores, students, utcnow
from .scoring import CATEGORY

log = logging.getLogger("forge.engine")


def _clean(v):
    return str(v or "").replace("\t", " ").replace("\n", " ").replace("\r", " ")


def _iso(dt):
    return dt.strftime("%Y-%m-%dT%H:%M:%S") if dt else ""


class EngineError(RuntimeError):
    pass


class ForgeEngine:
    """C++ forge_engine ka long-running process. Ek lock = ek command ek time pe."""

    def __init__(self):
        self.proc = None
        self.lock = threading.Lock()
        # cache[category][student_id] = (exp, rank, prev_exp, prev_rank)
        self.cache = {}
        self.counts = {}
        self.computed_at = None

    def ensure_binary(self):
        if ENGINE_PATH.exists():
            return
        gxx = shutil.which("g++")
        if not gxx:
            raise EngineError(f"C++ engine not found at {ENGINE_PATH} and g++ is not installed to build it.")
        log.info("Building C++ engine with %s", gxx)
        subprocess.run([gxx, "-std=c++17", "-O2", str(ENGINE_SOURCE), "-o", str(ENGINE_PATH)], check=True)

    def start(self):
        self.ensure_binary()
        self.proc = subprocess.Popen(
            [str(ENGINE_PATH)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
            text=True, encoding="utf-8", bufsize=1,
        )
        if self._call("PING") != ["PONG"]:
            raise EngineError("C++ engine did not answer PING.")

    def stop(self):
        if self.proc and self.proc.poll() is None:
            try:
                self.proc.stdin.write("QUIT\n")
                self.proc.stdin.flush()
                self.proc.wait(timeout=3)
            except Exception:
                self.proc.kill()

    def _call(self, command):
        # Command bhejo, "END" tak lines padho. Process mar gaya toh dobara start
        with self.lock:
            if self.proc is None or self.proc.poll() is not None:
                self.proc = subprocess.Popen(
                    [str(ENGINE_PATH)], stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL,
                    text=True, encoding="utf-8", bufsize=1,
                )
            self.proc.stdin.write(command + "\n")
            self.proc.stdin.flush()
            lines = []
            while True:
                line = self.proc.stdout.readline()
                if not line:
                    raise EngineError("C++ engine stopped unexpectedly.")
                line = line.rstrip("\n")
                if line == "END":
                    break
                lines.append(line)
        if lines and lines[0].startswith("ERR"):
            raise EngineError(lines[0])
        return lines

    def recompute(self):
        """DB se export -> C++ LOAD -> RANK -> scores table + memory cache."""
        CACHE_DIR.mkdir(parents=True, exist_ok=True)
        s_path, a_path = CACHE_DIR / "students.tsv", CACHE_DIR / "achievements.tsv"
        with db.connect() as c:
            srows = c.execute(select(students).where(students.c.role == "student").order_by(students.c.id)).mappings().all()
            arows = c.execute(
                select(achievements).where(achievements.c.status == "approved").order_by(achievements.c.id)
            ).mappings().all()
        with open(s_path, "w", encoding="utf-8", newline="\n") as f:
            for s in srows:
                f.write("\t".join(_clean(x) for x in (
                    s["id"], s["name"], s["roll_no"], s["branch"], s["grad_year"], s["campus"],
                    ";".join(s["skills"] or []), s["headline"])) + "\n")
        with open(a_path, "w", encoding="utf-8", newline="\n") as f:
            for a in arows:
                f.write("\t".join(_clean(x) for x in (
                    a["id"], a["student_id"], CATEGORY[a["category"]]["cpp"], a["achieved_on"].isoformat(),
                    a["score"], a["platform"], _iso(a["reviewed_at"]))) + "\n")

        as_of = _iso(utcnow() - timedelta(days=7))
        self._call(f"LOAD\t{s_path}\t{a_path}\t{as_of}")
        cache, counts, rows = {}, {}, []
        for line in self._call("RANK"):
            cat, sid, exp, rank, pexp, prank = line.split("\t")
            sid, rank, prank = int(sid), int(rank), int(prank)
            entry = (float(exp), rank, float(pexp), prank if prank > 0 else None)
            cache.setdefault(cat, {})[sid] = entry
            counts[cat] = counts.get(cat, 0) + 1
            rows.append({"student_id": sid, "category": cat, "exp": entry[0], "rank": rank,
                         "prev_exp": entry[2], "prev_rank": entry[3], "updated_at": utcnow()})
        with db.begin() as c:
            c.execute(delete(scores))
            if rows:
                c.execute(insert(scores), rows)
        self.cache, self.counts, self.computed_at = cache, counts, datetime.now()

    def fresh(self):
        # "Pichhle hafte" wala comparison purana na ho jaye, isliye ghante mein ek baar refresh
        if self.computed_at is None or datetime.now() - self.computed_at > timedelta(hours=1):
            self.recompute()

    def get(self, category, sid):
        return self.cache.get(category, {}).get(sid)

    def board(self, category):
        return self.cache.get(category, {})

    def search(self, query, k=500):
        q = _clean(query)[:100]
        if not q.strip():
            return []
        out = []
        for line in self._call(f"SEARCH\t{k}\t{q}"):
            sid, rel = line.split("\t")
            out.append((int(sid), int(rel)))
        return out


forge = ForgeEngine()
