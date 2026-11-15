import { useCallback, useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import * as api from '../api/index.js';
import { useAuth } from '../context/AuthContext.jsx';
import { useShell } from '../components/AppShell.jsx';
import Icon, { CATEGORY_ICON } from '../components/Icon.jsx';
import { Avatar, ExpDigits, RankNumber, TierHex, Progress, Skeleton, ErrorState, Empty, Count } from '../components/ui.jsx';
import { fmt, firstName, greeting, isoWeek, shortDate } from '../lib/format.js';
import { CATEGORY, MAX_EXP } from '../lib/scoring.js';
import { useDocumentTitle } from '../lib/hooks.js';

export default function Dashboard() {
  const { studentId, dataVersion } = useAuth();
  const { openAdd } = useShell();
  const [d, setD] = useState(null);
  const [error, setError] = useState(null);
  useDocumentTitle('Dashboard');

  const load = useCallback(() => {
    setError(null);
    api.getDashboard(studentId).then(setD).catch(setError);
  }, [studentId]);
  useEffect(load, [load, dataVersion]);

  if (error) return <ErrorState error={error} onRetry={load} />;
  if (!d) return <DashboardSkeleton />;

  const now = new Date();
  const s = d.student;
  const weekDelta = d.exp - d.expWeekAgo;

  return (
    <div className="dash">
      <div className="dash-hello enter">
        <div>
          <span className="label">Week {isoWeek(now)} · GEHU {s.campus}</span>
          <h1>{greeting(now)}, {firstName(s.name)}</h1>
        </div>
        {d.pending > 0 && (
          <Link to="/app/profile#achievements" className="pill pending-pill">
            <span className="dot-live" aria-hidden="true" /> {d.pending} in review
          </Link>
        )}
      </div>

      <section className="panel board enter enter-2" aria-label="Your standing">
        <div className="seg seg-rank">
          <span className="label">College rank · Overall</span>
          <RankNumber rank={d.rank} />
          {d.rank ? (
            <MovePill move={d.move} />
          ) : (
            <span className="pill">Not ranked yet</span>
          )}
        </div>
        <div className="seg seg-exp">
          <span className="label">EXP</span>
          {weekDelta > 0.5 && <span className="gain mono">+{fmt(weekDelta)} this week</span>}
          <div className="exp-row">
            <ExpDigits value={d.exp} />
            <span className="of mono">/ {fmt(MAX_EXP)}</span>
          </div>
          <span className="seg-foot">{d.top ? <>Top {d.top}% of {d.ranked} ranked students</> : <>{d.ranked} students are ranked</>}</span>
        </div>
        <div className="seg seg-tier">
          <div className="tier-row">
            <TierHex tier={d.tier} />
            <div>
              <b className="tier-name" style={{ color: d.tier.color, fontSize: 34 }}>{d.tier.name}</b>
              <small>Tier {['Candidate', 'Specialist', 'Expert', 'Master', 'Grandmaster'].indexOf(d.tier.name) + 1} of 5</small>
            </div>
          </div>
          <div>
            <Progress value={d.nextTier ? (d.exp - d.tier.min) / (d.nextTier.min - d.tier.min) : 1} color={d.tier.color} label={`Progress to ${d.nextTier?.name || 'max tier'}`} />
            <div className="tier-foot">
              {d.nextTier ? (
                <span><b className="num">{fmt(d.nextTier.min - d.exp)}</b> EXP to <em style={{ color: d.nextTier.color }}>{d.nextTier.name}</em></span>
              ) : (
                <span>Top tier reached</span>
              )}
              <span className="num">{fmt(d.nextTier?.min || MAX_EXP)}</span>
            </div>
          </div>
        </div>
        <div className="seg seg-streak">
          <div className="streak-head">
            <svg className="flame" viewBox="0 0 26 30" aria-hidden="true">
              <path d="M13 2c1 6 9 9 9 17a9 9 0 0 1-18 0c0-5 3-7 4-11 1 3 2 4 4 5-1-4 0-8 1-11z" fill={d.weekStreak ? '#FF6B4A' : '#383C42'} />
              <path d="M13 16c1 3 4 4 4 7a4 4 0 0 1-8 0c0-2 1-3 2-5 .5 1 1 1.5 2 2 0-1.5-.3-2.6 0-4z" fill={d.weekStreak ? '#FFC93C' : '#24272C'} />
            </svg>
            <b className="display">{d.weekStreak ? `${d.weekStreak}-WEEK STREAK` : 'NO STREAK YET'}</b>
          </div>
          <WeekDots activity={d.activity} now={now} />
        </div>
      </section>

      <div className="dash-grid">
        <section className="panel panel-pad stand enter enter-3" aria-labelledby="stand-h">
          <div className="panel-head">
            <h2 id="stand-h">Where you stand</h2>
            <span className="pill">vs {d.totalStudents} students</span>
          </div>
          <ul className="cats">
            {d.categories.map((c) => (
              <li key={c.key} className="cat">
                <span className="nm">
                  <Icon name={CATEGORY_ICON[c.key]} />
                  {c.label}
                  <small className="mono">×{c.weight}</small>
                </span>
                <CatBar c={c} />
                <span className={`pct ${!c.top || c.top > 50 ? 'lo' : ''}`}>{c.top ? `Top ${c.top}%` : 'Not started'}</span>
              </li>
            ))}
          </ul>
          <Summary cats={d.categories} />
        </section>

        <section className="panel panel-pad enter enter-4" aria-labelledby="ms-h">
          <div className="panel-head">
            <h2 id="ms-h">Next milestones</h2>
          </div>
          <ul className="milestones">
            {d.milestones.map((m, i) => (
              <li key={i} className="mi">
                <span className="ic"><Icon name={m.kind === 'pass' ? 'up' : m.kind === 'tier' ? 'star' : m.kind === 'start' ? 'bolt' : CATEGORY_ICON[m.category]} strokeWidth={2.4} /></span>
                <div>
                  <b>{m.title}</b>
                  <span>{m.detail}</span>
                  <Progress value={m.progress} />
                </div>
              </li>
            ))}
          </ul>
          <button className="btn btn-ghost btn-block" onClick={() => openAdd({ category: d.milestones.find((m) => m.category)?.category })} style={{ marginTop: 14 }}>
            <Icon name="plus" strokeWidth={2.6} /> Add an achievement
          </button>
        </section>

        <section className="panel panel-pad mini-lb enter enter-5" aria-labelledby="lb-h">
          <div className="panel-head">
            <h2 id="lb-h">College leaderboard</h2>
            <Link to="/app/search" className="more">See all <Icon name="arrow" size={14} /></Link>
          </div>
          <MiniBoard board={d.board} youId={s.id} />
        </section>
      </div>

      <div className="dash-grid two">
        <section className="panel panel-pad enter enter-4" aria-labelledby="act-h">
          <div className="panel-head">
            <h2 id="act-h">Activity</h2>
            <span className="muted" style={{ fontSize: 12, fontWeight: 700 }}>Last 12 weeks</span>
          </div>
          <Heatmap activity={d.activity} />
        </section>
        <section className="panel panel-pad enter enter-5" aria-labelledby="recent-h">
          <div className="panel-head">
            <h2 id="recent-h">Recent achievements</h2>
            <Link to="/app/profile#achievements" className="more">All <Icon name="arrow" size={14} /></Link>
          </div>
          {d.recent.length === 0 ? (
            <Empty icon="trophy" title="Nothing here yet" action={<button className="btn btn-primary btn-sm" onClick={() => openAdd()}>Add your first</button>}>
              Add a certificate, contest, project or internship. It counts once a reviewer approves it.
            </Empty>
          ) : (
            <ul className="recent">
              {d.recent.map((a) => (
                <li key={a.id}>
                  <span className="r-ic"><Icon name={CATEGORY_ICON[a.category]} /></span>
                  <div className="r-main">
                    <b>{a.title}</b>
                    <small>{CATEGORY[a.category].label}{a.organization ? ` · ${a.organization}` : ''} · {shortDate(a.achieved_on)}</small>
                  </div>
                  <span className={`status status-${a.status}`}>{a.status}</span>
                </li>
              ))}
            </ul>
          )}
        </section>
      </div>
    </div>
  );
}

function MovePill({ move }) {
  if (!move) return <span className="pill">No change this week</span>;
  if (move.kind === 'new') return <span className="pill delta-up">New on the board</span>;
  if (move.value > 0) return <span className="pill delta-up">▲ {move.value} this week</span>;
  if (move.value < 0) return <span className="pill delta-down">▼ {-move.value} this week</span>;
  return <span className="pill">No change this week</span>;
}

function CatBar({ c }) {
  const value = c.top ? 100 - c.top + 1 : 0;
  const gain = Math.min(c.gain || 0, value);
  return (
    <span className="bar" role="img" aria-label={c.top ? `${c.label}: top ${c.top} percent` : `${c.label}: not started`}>
      <Progress value={value / 100} />
      {gain > 0 && <i className="gain-seg" style={{ left: `${value - gain}%`, width: `${gain}%` }} />}
    </span>
  );
}

function Summary({ cats }) {
  const ranked = cats.filter((c) => c.top);
  const strongest = ranked.slice().sort((a, b) => a.top - b.top)[0];
  const focus = cats.filter((c) => c.weight >= 2.5).sort((a, b) => (b.top ?? 101) - (a.top ?? 101))[0];
  const best = cats.slice().sort((a, b) => (b.gain || 0) - (a.gain || 0))[0];
  return (
    <div className="sum">
      <div><span className="label">Strongest</span><b>{strongest?.short || '—'}</b></div>
      <div><span className="label">Focus next</span><b className="coral">{focus?.short || '—'}</b></div>
      <div><span className="label">This week</span><b className="lime">{best?.gain ? `+${Math.min(best.gain, 20)}% ${best.short}` : 'Steady'}</b></div>
    </div>
  );
}

function WeekDots({ activity, now }) {
  const weeks = [];
  for (let w = 7; w >= 0; w--) {
    const slice = activity.slice(Math.max(0, activity.length - (w + 1) * 7), activity.length - w * 7);
    weeks.push({ n: isoWeek(new Date(now - w * 7 * 86400000)), on: slice.some((x) => x.count > 0) });
  }
  return (
    <ol className="weeks" aria-label="Weeks with a submission">
      {weeks.map((w, i) => (
        <li key={i} className={w.on ? 'on' : ''} style={{ animationDelay: `${0.5 + i * 0.08}s` }}>
          <span className="wdot" aria-hidden="true">{w.on && <Icon name="check" strokeWidth={3.4} />}</span>
          <span>W{w.n}</span>
          <span className="sr-only">{w.on ? 'active' : 'no submissions'}</span>
        </li>
      ))}
    </ol>
  );
}

function MiniBoard({ board, youId }) {
  const Row = ({ r, delay }) => (
    <li className={`lb-row ${r.rank <= 3 ? 'top' : ''} ${r.id === youId ? 'you' : ''}`} style={{ animationDelay: `${delay}s` }}>
      <span className="rk">{r.rank}</span>
      <Avatar student={r} size={32} />
      <span className="who">
        {r.hidden ? <b>{r.name}</b> : <Link to={r.id === youId ? '/app/profile' : `/app/students/${r.id}`}><b>{r.id === youId ? `${r.name} (you)` : r.name}</b></Link>}
        <small>{r.branch} · {r.grad_year}</small>
      </span>
      <span />
      <span className="xp">{fmt(r.exp)}</span>
    </li>
  );
  return (
    <ol className="lb-list">
      {board.top.map((r, i) => <Row key={r.id} r={r} delay={0.3 + i * 0.05} />)}
      {board.gap && <li className="lb-gap" aria-hidden="true">· · ·</li>}
      {board.around.map((r, i) => <Row key={r.id} r={r} delay={0.45 + i * 0.05} />)}
      {board.around.length === 0 && board.top.length === 0 && <li className="muted">No ranked students yet.</li>}
    </ol>
  );
}

function Heatmap({ activity }) {
  const max = Math.max(1, ...activity.map((a) => a.count));
  const first = new Date(activity[0].date);
  const pad = (first.getDay() + 6) % 7;
  const cells = [...Array(pad).fill(null), ...activity];
  const total = activity.reduce((n, a) => n + a.count, 0);
  const activeDays = activity.filter((a) => a.count > 0).length;
  const last30 = activity.slice(-30).reduce((n, a) => n + a.count, 0);
  return (
    <div className="heat-wrap">
      <div className="heat-row">
      <div className="heat" role="img" aria-label={`${total} submissions in the last 12 weeks`}>
        {cells.map((c, i) =>
          c ? (
            <span key={i} className="cell" data-l={c.count === 0 ? 0 : Math.min(4, Math.ceil((c.count / max) * 4))} title={`${shortDate(c.date)}: ${c.count} submission${c.count === 1 ? '' : 's'}`} />
          ) : (
            <span key={i} className="cell blank" />
          )
        )}
      </div>
      <dl className="heat-stats">
        <div><dt className="label">Submissions</dt><dd className="display"><Count value={total} /></dd></div>
        <div><dt className="label">Active days</dt><dd className="display"><Count value={activeDays} /></dd></div>
        <div><dt className="label">Last 30 days</dt><dd className="display"><Count value={last30} /></dd></div>
      </dl>
      </div>
      <div className="heat-foot">
        <span>Each square is one day. Brighter means more submissions.</span>
        <span className="legend" aria-hidden="true">Less <i data-l="0" /><i data-l="1" /><i data-l="2" /><i data-l="3" /><i data-l="4" /> More</span>
      </div>
    </div>
  );
}

function DashboardSkeleton() {
  return (
    <div className="dash" aria-busy="true" aria-label="Loading dashboard">
      <Skeleton h={46} w={320} />
      <div className="panel board"><div className="seg" style={{ gridColumn: '1 / -1' }}><Skeleton h={120} /></div></div>
      <div className="dash-grid">
        {[0, 1, 2].map((i) => <div key={i} className="panel panel-pad"><Skeleton h={22} w="50%" /><div style={{ height: 16 }} />{[0, 1, 2, 3, 4].map((j) => <div key={j} style={{ marginBottom: 14 }}><Skeleton h={14} /></div>)}</div>)}
      </div>
    </div>
  );
}

