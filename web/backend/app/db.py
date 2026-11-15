from datetime import datetime, timezone

from sqlalchemy import (JSON, Boolean, CheckConstraint, Column, Date, DateTime, Float, ForeignKey, Index, Integer,
                        MetaData, String, Table, Text, create_engine, event, text)

from .config import DATABASE_URL

metadata = MetaData()


def utcnow():
    return datetime.now(timezone.utc).replace(tzinfo=None)


# Saare DB rules yahin: SQLite (local) aur Postgres (Supabase) dono pe chalte hain
students = Table(
    "students", metadata,
    Column("id", Integer, primary_key=True, autoincrement=True),
    Column("roll_no", String(10), nullable=False, unique=True),
    Column("name", String(80), nullable=False),
    Column("email", String(254), nullable=False, unique=True),
    Column("password_hash", String(100)),
    Column("role", String(10), nullable=False, default="student"),
    Column("course", String(20), nullable=False, default="B.Tech"),
    Column("branch", String(10), nullable=False),
    Column("section", String(1)),
    Column("campus", String(20), nullable=False, default="Dehradun"),
    Column("grad_year", Integer, nullable=False),
    Column("headline", String(80)),
    Column("bio", String(200)),
    Column("avatar_url", Text),
    Column("skills", JSON, nullable=False, default=list),
    Column("github", String(60)),
    Column("leetcode", String(60)),
    Column("codeforces", String(60)),
    Column("codechef", String(60)),
    Column("linkedin", String(200)),
    Column("cf_rating", Integer),
    Column("dsa_easy", Integer, nullable=False, default=0),
    Column("dsa_medium", Integer, nullable=False, default=0),
    Column("dsa_hard", Integer, nullable=False, default=0),
    Column("show_profile", Boolean, nullable=False, default=True),
    Column("show_achievements", Boolean, nullable=False, default=True),
    Column("show_rank", Boolean, nullable=False, default=True),
    Column("created_at", DateTime, nullable=False, default=utcnow),
    Column("updated_at", DateTime, nullable=False, default=utcnow, onupdate=utcnow),
    CheckConstraint("length(roll_no) = 10", name="roll_no_10_digits"),
    CheckConstraint("role in ('student', 'admin')", name="role_valid"),
    CheckConstraint("branch in ('CSE', 'IT', 'ECE', 'AIML', 'DS', 'STAFF')", name="branch_valid"),
    CheckConstraint("grad_year between 2000 and 2040", name="grad_year_valid"),
    CheckConstraint("dsa_easy >= 0 and dsa_medium >= 0 and dsa_hard >= 0", name="dsa_non_negative"),
)

achievements = Table(
    "achievements", metadata,
    Column("id", Integer, primary_key=True, autoincrement=True),
    Column("student_id", Integer, ForeignKey("students.id", ondelete="CASCADE"), nullable=False),
    Column("category", String(20), nullable=False),
    Column("title", String(120), nullable=False),
    Column("organization", String(80)),
    Column("description", String(300)),
    Column("achieved_on", Date, nullable=False),
    Column("score", Integer, nullable=False),
    Column("platform", String(20)),
    Column("proof_url", String(500), nullable=False),
    Column("status", String(10), nullable=False, default="pending"),
    Column("points", Float),
    Column("points_applied", Boolean, nullable=False, default=False),
    Column("review_note", String(300)),
    Column("reviewed_by", Integer, ForeignKey("students.id", ondelete="SET NULL")),
    Column("auto_check", JSON),
    Column("submitted_at", DateTime, nullable=False, default=utcnow),
    Column("reviewed_at", DateTime),
    CheckConstraint("category in ('competitive', 'dsa', 'project', 'internship', 'certification', 'academic')", name="category_valid"),
    CheckConstraint("status in ('pending', 'approved', 'rejected')", name="status_valid"),
    CheckConstraint("score between 0 and 1000", name="score_range"),
    CheckConstraint("category <> 'competitive' or platform is not null", name="competitive_needs_platform"),
    CheckConstraint("platform is null or category in ('competitive', 'dsa')", name="platform_only_for_coding"),
    CheckConstraint("(status = 'pending') = (reviewed_at is null)", name="reviewed_at_matches_status"),
    CheckConstraint("points_applied = false or status = 'approved'", name="points_only_when_approved"),
)
Index("achievements_student_idx", achievements.c.student_id, achievements.c.submitted_at)
Index("achievements_queue_idx", achievements.c.status, achievements.c.submitted_at)

# C++ engine likhta hai, website sirf padhti hai
scores = Table(
    "scores", metadata,
    Column("student_id", Integer, ForeignKey("students.id", ondelete="CASCADE"), primary_key=True),
    Column("category", String(20), primary_key=True),
    Column("exp", Float, nullable=False),
    Column("rank", Integer, nullable=False),
    Column("prev_exp", Float, nullable=False, default=0),
    Column("prev_rank", Integer),
    Column("updated_at", DateTime, nullable=False, default=utcnow),
)

# Har din jab student platform kholta hai: ek row. Login streak aur "active days" isi se
activity = Table(
    "activity", metadata,
    Column("student_id", Integer, ForeignKey("students.id", ondelete="CASCADE"), primary_key=True),
    Column("day", Date, primary_key=True),
)

engine = create_engine(DATABASE_URL, future=True, pool_pre_ping=True)

if engine.dialect.name == "sqlite":
    @event.listens_for(engine, "connect")
    def _sqlite_pragmas(conn, _):
        cur = conn.cursor()
        cur.execute("PRAGMA foreign_keys = ON")
        cur.execute("PRAGMA journal_mode = WAL")
        cur.close()


def create_all():
    metadata.create_all(engine)


def reset_sequences(conn):
    # Postgres mein explicit ids daalne ke baad sequence aage badhana padta hai
    if engine.dialect.name == "postgresql":
        for table in ("students", "achievements"):
            conn.execute(text(f"select setval(pg_get_serial_sequence('{table}', 'id'), coalesce(max(id), 1)) from {table}"))
