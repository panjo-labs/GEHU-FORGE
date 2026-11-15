from datetime import datetime, timedelta, timezone

import bcrypt
import jwt
from fastapi import Depends, HTTPException
from fastapi.security import HTTPAuthorizationCredentials, HTTPBearer
from sqlalchemy import select

from .config import JWT_SECRET, TOKEN_DAYS
from .activity import mark_active
from .db import engine, students
from .engine import forge

bearer = HTTPBearer(auto_error=False)


def hash_password(password):
    return bcrypt.hashpw(password.encode(), bcrypt.gensalt(rounds=12)).decode()


def verify_password(password, hashed):
    if not hashed:
        return False
    try:
        return bcrypt.checkpw(password.encode(), hashed.encode())
    except ValueError:
        return False


def make_token(student_id, role):
    now = datetime.now(timezone.utc)
    payload = {"sub": str(student_id), "role": role, "iat": now, "exp": now + timedelta(days=TOKEN_DAYS)}
    return jwt.encode(payload, JWT_SECRET, algorithm="HS256")


def current_user(creds: HTTPAuthorizationCredentials | None = Depends(bearer)):
    if creds is None:
        raise HTTPException(401, "Sign in to continue.")
    try:
        payload = jwt.decode(creds.credentials, JWT_SECRET, algorithms=["HS256"])
        uid = int(payload["sub"])
    except (jwt.PyJWTError, KeyError, ValueError):
        raise HTTPException(401, "Your session has expired. Sign in again.")
    # Role DB se padho, token pe bharosa nahi (role badla ho sakta hai)
    with engine.connect() as c:
        row = c.execute(select(students.c.id, students.c.role).where(students.c.id == uid)).first()
    if row is None:
        raise HTTPException(401, "Your account was not found. Sign in again.")
    # Aaj pehli baar aaya -> streak badla, agli baar rank dobara nikalo
    if row.role == "student" and mark_active(row.id):
        forge.invalidate()
    return {"id": row.id, "role": row.role}


def optional_user(creds: HTTPAuthorizationCredentials | None = Depends(bearer)):
    if creds is None:
        return None
    try:
        return current_user(creds)
    except HTTPException:
        return None


def require_admin(user=Depends(current_user)):
    if user["role"] != "admin":
        raise HTTPException(403, "Only reviewers can open this page.")
    return user
