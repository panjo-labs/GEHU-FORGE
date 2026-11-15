import { useCallback, useEffect, useLayoutEffect, useRef, useState } from 'react';
import { Link, useNavigate, useSearchParams } from 'react-router-dom';
import * as api from '../api/index.js';
import { useAuth } from '../context/AuthContext.jsx';
import Icon from '../components/Icon.jsx';
import { Avatar, Tabs, TierTag, MoveTag, StudentPreview, Skeleton, Empty, ErrorState } from '../components/ui.jsx';
import { CATEGORIES, TIERS } from '../lib/scoring.js';
import { BRANCHES, CAMPUSES, GRAD_YEARS } from '../lib/validate.js';
import { fmt } from '../lib/format.js';
import { useDocumentTitle, usePrefersReducedMotion } from '../lib/hooks.js';

const CAT_TABS = [{ value: 'all', label: 'Overall' }, ...CATEGORIES.map((c) => ({ value: c.key, label: c.short }))];
const PAGE = 40;

export default function Search() {
  const { studentId, dataVersion } = useAuth();
  const [params, setParams] = useSearchParams();
  const navigate = useNavigate();
  const category = params.get('category') || 'all';
  const q = params.get('q') || '';
  const branch = params.get('branch') || '';
  const batch = params.get('batch') || '';
  const campus = params.get('campus') || '';
  const tier = params.get('tier') || '';
  const [query, setQuery] = useState(q);
  const [res, setRes] = useState(null);
  const [error, setError] = useState(null);
  const [shown, setShown] = useState(PAGE);
  const [hover, setHover] = useState(null);
  const [showFilters, setShowFilters] = useState(Boolean(branch || batch || campus || tier));
  useDocumentTitle('Leaderboard');

  useEffect(() => setQuery(q), [q]);

  const update = useCallback(
    (patch) => {
      const next = new URLSearchParams(params);
      for (const [k, v] of Object.entries(patch)) {
        if (v) next.set(k, v);
        else next.delete(k);
      }
      setParams(next, { replace: true });
      setShown(PAGE);
    },
    [params, setParams]
  );

  // Type karte hi search, 250ms debounce
  useEffect(() => {
    if (query === q) return;
    const t = setTimeout(() => update({ q: query.trim() }), 250);
    return () => clearTimeout(t);
  }, [query, q, update]);

  const load = useCallback(() => {
    setError(null);
    api.getLeaderboard({ category, q, branch, batch, campus, tier, viewerId: studentId }).then(setRes).catch(setError);
  }, [category, q, branch, batch, campus, tier, studentId]);
  useEffect(load, [load, dataVersion]);

  const filtersOn = [branch, batch, campus, tier].filter(Boolean).length;
  const rows = res?.rows || [];
  const podium = res && !res.filtered && rows.length >= 3 ? rows.slice(0, 3) : null;
  const list = podium ? rows.slice(3, shown) : rows.slice(0, shown);
  const meInList = rows.slice(0, shown).some((r) => r.self);
  const catLabel = CAT_TABS.find((t) => t.value === category)?.label;

  return (
    <div className="search-page">
      <div className="page-head enter">
        <div>
          <h1>College leaderboard</h1>
          <p>
            {!res ? 'Loading rankings…' : res.searched ? `${res.rows.length} matches for “${q}”, best match first` : `${res.total} ranked students in ${catLabel === 'Overall' ? 'the overall table' : catLabel}`}
          </p>
        </div>
        <Tabs items={CAT_TABS} value={category} onChange={(v) => update({ category: v === 'all' ? '' : v })} label="Ranking category" />
      </div>

      <div className="search-bar panel enter enter-1">
        <div className="search big">
          <Icon name="search" />
          <label htmlFor="lb-search" className="sr-only">Search by name, roll number or skill</label>
          <input id="lb-search" value={query} onChange={(e) => setQuery(e.target.value)} placeholder="Search by name, roll number or skill (e.g. React)" autoComplete="off" />
          {query && (
            <button className="icon-btn" onClick={() => { setQuery(''); update({ q: '' }); }} aria-label="Clear search">
              <Icon name="x" />
            </button>
          )}
        </div>
        <button className={`btn btn-ghost ${filtersOn ? 'has' : ''}`} onClick={() => setShowFilters((x) => !x)} aria-expanded={showFilters} aria-controls="lb-filters">
          <Icon name="filter" /> Filters{filtersOn ? ` · ${filtersOn}` : ''}
        </button>
      </div>

      {showFilters && (
        <div className="filters enter" id="lb-filters">
          <FilterSelect id="f-branch" label="Branch" value={branch} onChange={(v) => update({ branch: v })} options={BRANCHES} />
          <FilterSelect id="f-batch" label="Batch" value={batch} onChange={(v) => update({ batch: v })} options={GRAD_YEARS.map(String)} />
          <FilterSelect id="f-campus" label="Campus" value={campus} onChange={(v) => update({ campus: v })} options={CAMPUSES} />
          <FilterSelect id="f-tier" label="Tier" value={tier} onChange={(v) => update({ tier: v })} options={TIERS.map((t) => t.name)} />
          {filtersOn > 0 && <button className="btn btn-quiet" onClick={() => update({ branch: '', batch: '', campus: '', tier: '' })}>Clear filters</button>}
        </div>
      )}

      {error && <ErrorState error={error} onRetry={load} />}
      {!res && !error && <ListSkeleton />}

      {res && (
        <>
          {podium && <Podium rows={podium} category={category} youId={studentId} />}

          <section className="panel lb-panel enter enter-2" aria-label="Rankings">
            <div className="lb-headrow" aria-hidden="true">
              <span>Rank</span><span /><span>Student</span><span className="hide-xs">Tier</span><span className="hide-xs">Week</span><span>{category === 'all' ? 'EXP' : `${catLabel} EXP`}</span>
            </div>
            {rows.length === 0 ? (
              <Empty icon="search" title="No students match" action={<button className="btn btn-ghost btn-sm" onClick={() => { setQuery(''); update({ q: '', branch: '', batch: '', campus: '', tier: '' }); }}>Clear search</button>}>
                Try a different name, skill or roll number, or remove a filter.
              </Empty>
            ) : (
              <FlipList
                rows={list}
                category={category}
                onHover={setHover}
                onOpen={(r) => navigate(r.self ? '/app/profile' : `/app/students/${r.id}`)}
                showZone={!res.filtered && !podium ? false : !res.filtered}
                total={res.total}
              />
            )}
            {rows.length > (podium ? 3 : 0) + list.length && (
              <div className="more-row">
                <button className="btn btn-ghost" onClick={() => setShown((n) => n + PAGE)}>Show {Math.min(PAGE, rows.length - shown)} more</button>
              </div>
            )}
          </section>

          {res.me && !meInList && !(podium && podium.some((r) => r.self)) && (
            <div className="me-pin" aria-label="Your position">
              <Row r={res.me} category={category} onHover={() => {}} onOpen={() => navigate('/app/profile')} />
            </div>
          )}
        </>
      )}

      {hover && <StudentPreview row={hover.row} style={{ position: 'fixed', top: hover.top, left: hover.left }} />}
    </div>
  );
}

function FilterSelect({ id, label, value, onChange, options }) {
  return (
    <div className="field">
      <label htmlFor={id}>{label}</label>
      <select id={id} className="select" value={value} onChange={(e) => onChange(e.target.value)}>
        <option value="">All</option>
        {options.map((o) => <option key={o} value={o}>{o}</option>)}
      </select>
    </div>
  );
}

function Podium({ rows, category, youId }) {
  const order = [rows[1], rows[0], rows[2]];
  return (
    <div className="podium enter enter-2">
      {order.map((r, i) => {
        const place = r.catRank;
        return (
          <Link key={r.id} to={r.id === youId ? '/app/profile' : `/app/students/${r.id}`} className={`pod pod-${place} ${r.id === youId ? 'you' : ''}`} style={{ animationDelay: `${0.15 + i * 0.08}s` }}>
            <span className="pod-place display">{place}</span>
            <Avatar student={r} size={place === 1 ? 72 : 58} />
            <b>{r.name}</b>
            <small>{r.branch} · {r.grad_year}</small>
            <TierTag tier={r.tier} />
            <span className="pod-exp mono">{fmt(category === 'all' ? r.exp : r.catExp)} EXP</span>
          </Link>
        );
      })}
    </div>
  );
}

function Row({ r, category, onHover, onOpen, style, rowRef }) {
  const enter = (e) => {
    if (window.matchMedia('(hover: none)').matches) return;
    const box = e.currentTarget.getBoundingClientRect();
    const left = Math.min(box.left + 220, window.innerWidth - 320);
    const top = Math.min(box.bottom + 6, window.innerHeight - 230);
    onHover({ row: r, top, left });
  };
  return (
    <li
      ref={rowRef}
      className={`lb-row ${r.catRank && r.catRank <= 3 ? 'top' : ''} ${r.self ? 'you' : ''}`}
      style={style}
      onMouseEnter={enter}
      onMouseLeave={() => onHover(null)}
    >
      <span className="rk">{r.catRank ?? '—'}</span>
      <Avatar student={r} size={36} />
      <span className="who">
        <a href={r.self ? '/app/profile' : `/app/students/${r.id}`} onClick={(e) => { e.preventDefault(); onOpen(r); }} onFocus={(e) => enter({ currentTarget: e.currentTarget.closest('li') })} onBlur={() => onHover(null)}>
          <b>{r.self ? `${r.name} (you)` : r.name}</b>
        </a>
        <small>{r.roll_no} · {r.branch} · {r.grad_year}{r.headline ? ` · ${r.headline}` : ''}</small>
      </span>
      <span className="hide-xs"><TierTag tier={r.tier} /></span>
      <span className="hide-xs"><MoveTag move={r.move} /></span>
      <span className="xp">{r.catRank ? fmt(category === 'all' ? r.exp : r.catExp) : <span className="muted">Unranked</span>}</span>
    </li>
  );
}

function FlipList({ rows, category, onHover, onOpen, showZone, total }) {
  const refs = useRef(new Map());
  const prev = useRef(new Map());
  const reduced = usePrefersReducedMotion();

  useLayoutEffect(() => {
    if (!reduced) {
      for (const [id, el] of refs.current) {
        if (!el) continue;
        const before = prev.current.get(id);
        const now = el.getBoundingClientRect().top;
        if (before !== undefined && Math.abs(before - now) > 1) {
          el.style.transition = 'none';
          el.style.transform = `translateY(${before - now}px)`;
          requestAnimationFrame(() => {
            el.style.transition = '';
            el.style.transform = '';
          });
        }
      }
    }
    const snap = new Map();
    for (const [id, el] of refs.current) if (el) snap.set(id, el.getBoundingClientRect().top);
    prev.current = snap;
  }, [rows, reduced]);

  const zoneAt = Math.max(1, Math.ceil(total * 0.05));
  return (
    <ol className="lb-full">
      {rows.map((r) => (
        <FragmentRow key={r.id} r={r} zoneAt={showZone ? zoneAt : null}>
          <Row r={r} category={category} onHover={onHover} onOpen={onOpen} rowRef={(el) => refs.current.set(r.id, el)} />
        </FragmentRow>
      ))}
    </ol>
  );
}

function FragmentRow({ r, zoneAt, children }) {
  return (
    <>
      {children}
      {zoneAt && r.catRank === zoneAt && <li className="lb-zone" aria-label="Top 5 percent line">TOP 5%</li>}
    </>
  );
}

function ListSkeleton() {
  return (
    <div className="panel lb-panel" aria-busy="true" aria-label="Loading leaderboard">
      {Array.from({ length: 8 }, (_, i) => (
        <div key={i} style={{ display: 'grid', gridTemplateColumns: '44px 36px 1fr 80px', gap: 12, alignItems: 'center', padding: '10px' }}>
          <Skeleton h={20} /><Skeleton h={36} r="50%" /><Skeleton h={14} w="60%" /><Skeleton h={14} />
        </div>
      ))}
    </div>
  );
}
