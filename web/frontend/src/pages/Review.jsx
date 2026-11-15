import { useCallback, useEffect, useRef, useState } from 'react';
import { Link } from 'react-router-dom';
import * as api from '../api/index.js';
import { useToast } from '../context/ToastContext.jsx';
import Icon, { CATEGORY_ICON } from '../components/Icon.jsx';
import { Avatar, Tabs, TierTag, Skeleton, Empty, ErrorState, Field } from '../components/ui.jsx';
import { CATEGORIES, CATEGORY, PLATFORM_MULTIPLIER } from '../lib/scoring.js';
import { fmt, shortDate } from '../lib/format.js';
import { useDocumentTitle } from '../lib/hooks.js';

const TABS = [{ value: '', label: 'All' }, ...CATEGORIES.map((c) => ({ value: c.key, label: c.short }))];

function waited(iso) {
  const h = Math.round((Date.now() - new Date(iso)) / 3600000);
  if (h < 1) return 'just now';
  if (h < 24) return `${h} h ago`;
  return `${Math.round(h / 24)} d ago`;
}

// Points = score × category weight × platform (sirf CP). C++ engine jaisa hi.
function pointsFor(a, score) {
  const mult = a.category === 'competitive' ? PLATFORM_MULTIPLIER[(a.platform || '').toLowerCase()] ?? 1 : 1;
  return { weight: CATEGORY[a.category].weight, mult, total: Math.round(score * CATEGORY[a.category].weight * mult * 100) / 100 };
}

export default function Review() {
  const toast = useToast();
  const [category, setCategory] = useState('');
  const [data, setData] = useState(null);
  const [stats, setStats] = useState(null);
  const [error, setError] = useState(null);
  const [selected, setSelected] = useState(null);
  const [page, setPage] = useState(1);
  const detailRef = useRef(null);
  useDocumentTitle('Review queue');

  const load = useCallback(() => {
    setError(null);
    setData(null);
    Promise.all([api.getPending({ page: 1, category }), api.getReviewStats()])
      .then(([q, s]) => {
        setData(q);
        setStats(s);
        setPage(1);
        setSelected(q.items[0]?.id ?? null);
      })
      .catch(setError);
  }, [category]);
  useEffect(load, [load]);

  async function more() {
    const next = await api.getPending({ page: page + 1, category });
    setData((d) => ({ ...d, items: [...d.items, ...next.items] }));
    setPage(page + 1);
  }

  function pick(id) {
    setSelected(id);
    if (window.matchMedia('(max-width: 1000px)').matches) requestAnimationFrame(() => detailRef.current?.scrollIntoView({ behavior: 'smooth' }));
  }

  function done(id, result) {
    // FIFO: decision ke baad agla sabse purana item apne aap khul jaata hai
    const idx = data.items.findIndex((x) => x.id === id);
    const items = data.items.filter((x) => x.id !== id);
    setData({ ...data, items, total: data.total - 1 });
    setSelected(items[Math.min(idx, items.length - 1)]?.id ?? null);
    setStats((s) => s && {
      ...s,
      pending: s.pending - 1,
      approved24h: s.approved24h + (result.status === 'approved' ? 1 : 0),
      rejected24h: s.rejected24h + (result.status === 'rejected' ? 1 : 0),
    });
    if (result.status === 'approved') {
      const gain = result.exp_after - result.exp_before;
      const moved = result.rank_before && result.rank_after && result.rank_before !== result.rank_after
        ? ` · #${result.rank_before} → #${result.rank_after}` : result.rank_after ? ` · now #${result.rank_after}` : '';
      toast({ title: 'Approved', body: `+${fmt(gain)} EXP${moved}` });
    } else if (result.status === 'rejected') {
      toast({ title: 'Rejected', body: 'The student sees your note on their profile.', tone: 'info' });
    }
  }

  const current = data?.items.find((x) => x.id === selected);

  return (
    <div className="review">
      <div className="page-head enter">
        <div>
          <h1>Review queue</h1>
          <p>Oldest first. Approve only what the proof shows.</p>
        </div>
        {stats && (
          <div className="rv-stats">
            <span className="pill pending-pill"><b className="num">{stats.pending}</b> pending</span>
            <span className="pill"><b className="num lime">{stats.approved24h}</b> approved · 24 h</span>
            <span className="pill"><b className="num coral">{stats.rejected24h}</b> rejected · 24 h</span>
            {stats.oldest && <span className="pill">Oldest waiting {waited(stats.oldest)}</span>}
          </div>
        )}
      </div>
      <Tabs items={TABS} value={category} onChange={setCategory} label="Filter by category" />

      {error && <ErrorState error={error} onRetry={load} />}
      {!data && !error && (
        <div className="rv-grid">
          <div className="panel rv-list">{[0, 1, 2, 3, 4].map((i) => <div key={i} style={{ padding: 12 }}><Skeleton h={40} /></div>)}</div>
          <div className="panel panel-pad"><Skeleton h={220} /></div>
        </div>
      )}

      {data && data.items.length === 0 && (
        <div className="panel"><Empty icon="check" title="The queue is clear">Nothing is waiting in this category. New submissions show up here first come, first served.</Empty></div>
      )}

      {data && data.items.length > 0 && (
        <div className="rv-grid">
          <section className="panel rv-list" aria-label="Pending submissions">
            <div className="rv-list-head"><span className="label">{data.total} waiting</span></div>
            <ol>
              {data.items.map((a, i) => (
                <li key={a.id}>
                  <button className={`rv-item ${a.id === selected ? 'on' : ''}`} onClick={() => pick(a.id)} aria-current={a.id === selected}>
                    <span className="rv-n mono">{i + 1}</span>
                    <span className="rv-ic"><Icon name={CATEGORY_ICON[a.category]} /></span>
                    <span className="rv-main">
                      <b>{a.title}</b>
                      <small>{a.student?.name} · {CATEGORY[a.category].short} · {waited(a.submitted_at)}</small>
                    </span>
                    <span className="rv-pts mono">{fmt(a.points_preview)}</span>
                  </button>
                </li>
              ))}
            </ol>
            {data.items.length < data.total && (
              <div className="more-row"><button className="btn btn-ghost btn-sm" onClick={more}>Load more</button></div>
            )}
          </section>
          <div ref={detailRef}>
            {current && <Detail key={current.id} a={current} onDone={done} />}
          </div>
        </div>
      )}
    </div>
  );
}

function Detail({ a, onDone }) {
  const toast = useToast();
  const [score, setScore] = useState(String(a.score));
  const [note, setNote] = useState('');
  const [busy, setBusy] = useState(null);
  const [error, setError] = useState(null);
  const [checks, setChecks] = useState(a.auto_check);
  const [checking, setChecking] = useState(false);
  const s = a.student;
  const n = Number(score);
  const valid = score !== '' && Number.isInteger(n) && n >= 0 && n <= 1000;
  const p = pointsFor(a, valid ? n : a.score);

  async function runChecks() {
    setChecking(true);
    try {
      setChecks((await api.recheckAchievement(a.id)).auto_check);
    } catch (e) {
      toast({ title: 'Checks did not run', body: e.message, tone: 'error' });
    } finally {
      setChecking(false);
    }
  }
  useEffect(() => {
    if (!a.auto_check) runChecks();
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [a.id]);

  async function decide(decision) {
    setError(null);
    if (decision === 'reject' && !note.trim()) return setError('Add a short note so the student knows why.');
    if (decision === 'approve' && !valid) return setError('Points must be a whole number from 0 to 1000.');
    setBusy(decision);
    try {
      const res = await api.reviewAchievement(a.id, { decision, note: note.trim() || null, score: decision === 'approve' ? n : undefined });
      onDone(a.id, res);
    } catch (e) {
      if (e.code === 'already_reviewed') {
        toast({ title: 'Already reviewed', body: 'Another reviewer decided this one first.', tone: 'info' });
        onDone(a.id, { status: 'skipped' });
      } else setError(e.message);
    } finally {
      setBusy(null);
    }
  }

  return (
    <section className="panel rv-detail enter" aria-labelledby="rv-title">
      <div className="rv-top">
        <span className="pill"><Icon name={CATEGORY_ICON[a.category]} size={14} /> {CATEGORY[a.category].label}</span>
        <span className="status status-pending">pending</span>
      </div>
      <h2 id="rv-title">{a.title}</h2>

      {s && (
        <Link to={`/app/students/${s.id}`} className="rv-student">
          <Avatar student={s} size={44} />
          <span>
            <b>{s.name}</b>
            <small className="mono">{s.roll_no} · {s.branch} · {s.grad_year} · {s.campus}</small>
          </span>
          <span className="rv-standing">
            {s.rank ? <b className="display">#{s.rank}</b> : <small>Unranked</small>}
            <TierTag tier={s.tier} />
          </span>
        </Link>
      )}

      <dl className="rv-facts">
        <div><dt>{a.category === 'internship' ? 'Company' : a.category === 'certification' ? 'Issued by' : 'Organisation'}</dt><dd>{a.organization || '—'}</dd></div>
        <div><dt>Date</dt><dd>{shortDate(a.achieved_on)}</dd></div>
        {a.platform && <div><dt>Platform</dt><dd>{a.platform}</dd></div>}
        <div><dt>Submitted</dt><dd>{waited(a.submitted_at)}</dd></div>
        {s?.github && <div><dt>GitHub</dt><dd className="mono">{s.github}</dd></div>}
        {s?.codeforces && <div><dt>Codeforces</dt><dd className="mono">{s.codeforces}</dd></div>}
      </dl>
      {a.description && <p className="rv-desc">{a.description}</p>}

      <a className="btn btn-ghost rv-proof" href={a.proof_url} target="_blank" rel="noreferrer noopener">
        <Icon name="link" /> Open proof <span className="mono">{new URL(a.proof_url).hostname}</span>
      </a>

      <div className="rv-checks">
        <div className="rv-checks-head">
          <span className="label">Automatic checks</span>
          <button className="btn btn-quiet btn-sm" onClick={runChecks} disabled={checking}>{checking ? 'Checking…' : 'Run again'}</button>
        </div>
        {!checks && checking && <Skeleton h={18} />}
        {checks && (
          <ul>
            {checks.map((c, i) => (
              <li key={i} className={`chk chk-${c.level}`}>
                <Icon name={c.level === 'ok' ? 'check' : c.level === 'warn' ? 'info' : 'search'} strokeWidth={2.6} />
                <span>{c.text}</span>
              </li>
            ))}
          </ul>
        )}
        <p className="hint">Checks use the free Codeforces and GitHub APIs. They help, but you make the decision.</p>
      </div>

      <div className="rv-decide">
        <div className="rv-points">
          <Field id="rv-score" label="Points" hint="Change it if the proof shows less than claimed" error={!valid ? 'Whole number, 0 to 1000' : undefined}>
            <input id="rv-score" type="number" min="0" max="1000" className="input mono" value={score} onChange={(e) => setScore(e.target.value)} />
          </Field>
          <div className="rv-formula" aria-label="Points after weights">
            <span className="mono">{valid ? n : a.score} × {p.weight}{p.mult !== 1 ? ` × ${p.mult}` : ''}</span>
            <b className="display">= {fmt(p.total)}</b>
            <small>weighted points</small>
          </div>
        </div>
        <Field id="rv-note" label="Note to the student" hint="Required when you reject">
          <textarea id="rv-note" className="textarea" maxLength={300} value={note} onChange={(e) => setNote(e.target.value)} placeholder="e.g. Certificate name does not match your profile." />
        </Field>
        {error && <p className="error" role="alert">{error}</p>}
        <div className="rv-actions">
          <button className="btn btn-danger" onClick={() => decide('reject')} disabled={!!busy}>
            <Icon name="x" /> {busy === 'reject' ? 'Rejecting…' : 'Reject'}
          </button>
          <button className="btn btn-primary" onClick={() => decide('approve')} disabled={!!busy}>
            <Icon name="check" strokeWidth={2.8} /> {busy === 'approve' ? 'Approving…' : 'Approve'}
          </button>
        </div>
      </div>
    </section>
  );
}
