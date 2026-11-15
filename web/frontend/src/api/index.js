// Backend (FastAPI) se baat karne ki ek hi jagah. Pages sirf yeh functions use karte hain.
import { TIERS } from '../lib/scoring.js';

const BASE = import.meta.env.VITE_API_URL || '/api';
const SESSION_KEY = 'forge.session.v2';

function apiError(code, message, errors) {
  const e = new Error(message);
  e.code = code;
  if (errors) e.errors = errors;
  return e;
}

// Server {name, min} bhejta hai; UI ko color/hex bhi chahiye
function hydrate(v) {
  if (Array.isArray(v)) return v.map(hydrate);
  if (v && typeof v === 'object') {
    const out = {};
    for (const [k, x] of Object.entries(v)) {
      out[k] = (k === 'tier' || k === 'nextTier') && x && x.name ? TIERS.find((t) => t.name === x.name) || x : hydrate(x);
    }
    return out;
  }
  return v;
}

function changed() {
  window.dispatchEvent(new Event('forge:data'));
}

async function request(path, { method = 'GET', body, auth = true } = {}) {
  const headers = { Accept: 'application/json' };
  const session = getSession();
  if (auth && session?.token) headers.Authorization = `Bearer ${session.token}`;
  if (body !== undefined) headers['Content-Type'] = 'application/json';
  let res;
  try {
    res = await fetch(BASE + path, { method, headers, body: body === undefined ? undefined : JSON.stringify(body) });
  } catch {
    throw apiError('network', 'Could not reach the server. Check your connection and try again.');
  }
  const data = res.status === 204 ? null : await res.json().catch(() => null);
  if (!res.ok) {
    const err = data?.error || {};
    if (res.status === 401 && session) {
      signOut();
      window.dispatchEvent(new Event('forge:logout'));
    }
    throw apiError(err.code || 'request_error', err.message || 'Something went wrong. Try again.', err.errors || (err.field ? { [err.field]: err.message } : null));
  }
  return hydrate(data);
}

// ---------------------------------------------------------------- auth

export function getSession() {
  try {
    return JSON.parse(localStorage.getItem(SESSION_KEY) || 'null');
  } catch {
    return null;
  }
}

function saveSession(s) {
  const session = { token: s.token, studentId: s.studentId, role: s.role };
  localStorage.setItem(SESSION_KEY, JSON.stringify(session));
  return session;
}

export function signOut() {
  localStorage.removeItem(SESSION_KEY);
}

export async function signIn({ email, password }) {
  return saveSession(await request('/auth/login', { method: 'POST', body: { email, password }, auth: false }));
}

export async function signInDemo(role = 'student') {
  return saveSession(await request('/auth/demo', { method: 'POST', body: { role }, auth: false }));
}

export async function signUp(form) {
  return saveSession(await request('/auth/signup', { method: 'POST', body: form, auth: false }));
}

// ---------------------------------------------------------------- reads

export const getMe = () => request('/me');
export const getLanding = () => request('/landing', { auth: false });
export const getDashboard = () => request('/dashboard');
export const getStudent = (id) => request(`/students/${id}`);

export function getLeaderboard({ category = 'all', q = '', branch = '', batch = '', campus = '', tier = '' } = {}) {
  const p = new URLSearchParams();
  for (const [k, v] of Object.entries({ category, q, branch, batch, campus, tier })) if (v) p.set(k, v);
  return request(`/leaderboard?${p}`);
}

// ---------------------------------------------------------------- writes

export async function updateProfile(_id, patch) {
  const res = await request('/me', { method: 'PUT', body: patch });
  changed();
  return res;
}

export async function addAchievement(_id, input) {
  const res = await request('/achievements', { method: 'POST', body: input });
  changed();
  return res;
}

export async function updateAchievement(_id, achId, input) {
  await request(`/achievements/${achId}`, { method: 'PUT', body: input });
  changed();
}

export async function deleteAchievement(_id, achId) {
  await request(`/achievements/${achId}`, { method: 'DELETE' });
  changed();
}

// ---------------------------------------------------------------- reviewer

export const getReviewStats = () => request('/admin/stats');

export function getPending({ page = 1, category = '' } = {}) {
  const p = new URLSearchParams({ page });
  if (category) p.set('category', category);
  return request(`/admin/pending?${p}`);
}

export async function reviewAchievement(achId, body) {
  const res = await request(`/admin/review/${achId}`, { method: 'POST', body });
  changed();
  return res;
}

export const recheckAchievement = (achId) => request(`/admin/recheck/${achId}`, { method: 'POST' });
