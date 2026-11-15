import { useCallback, useEffect, useState } from 'react';
import { Link, useLocation, useParams } from 'react-router-dom';
import * as api from '../api/index.js';
import { useAuth } from '../context/AuthContext.jsx';
import { useToast } from '../context/ToastContext.jsx';
import { useShell } from '../components/AppShell.jsx';
import Icon, { CATEGORY_ICON } from '../components/Icon.jsx';
import ProfileModal from '../components/ProfileModal.jsx';
import AchievementModal from '../components/AchievementModal.jsx';
import {
  Avatar, TierHex, Progress, Tabs, Skeleton, Empty, ErrorState, BadgeIcon, UnlockOverlay, Modal, GehuLogo, Count,
} from '../components/ui.jsx';
import { CATEGORIES, CATEGORY } from '../lib/scoring.js';
import { fmt, shortDate } from '../lib/format.js';
import { useDocumentTitle } from '../lib/hooks.js';

export function MyProfile() {
  const { studentId } = useAuth();
  return <ProfilePage id={studentId} />;
}

export function StudentProfile() {
  const { id } = useParams();
  return <ProfilePage id={Number(id)} />;
}

function ProfilePage({ id }) {
  const { studentId, dataVersion } = useAuth();
  const [d, setD] = useState(null);
  const [error, setError] = useState(null);
  const [editing, setEditing] = useState(false);
  const [sharing, setSharing] = useState(false);
  const [unlocked, setUnlocked] = useState([]);
  const location = useLocation();

  const load = useCallback(() => {
    setError(null);
    api.getStudent(id, studentId).then(setD).catch(setError);
  }, [id, studentId]);
  useEffect(load, [load, dataVersion]);
  useDocumentTitle(d?.student?.name || 'Profile');

  useEffect(() => {
    if (d && location.hash === '#achievements') document.getElementById('achievements')?.scrollIntoView({ behavior: 'smooth' });
  }, [d, location.hash]);

  if (error) return <ErrorState error={error} onRetry={load} />;
  if (d?.private && d.id === id) {
    return (
      <div className="panel panel-pad">
        <Empty icon="lock" title={`${d.name} keeps their profile private`}>
          Only their own account can see it. <Link to="/app/search" className="lime">Back to the leaderboard</Link>
        </Empty>
      </div>
    );
  }
  if (!d || d.private || d.student.id !== id) return <ProfileSkeleton />;

  const s = d.student;
  return (
    <div className="profile">
      <PlayerCard d={d} onEdit={() => setEditing(true)} onShare={() => setSharing(true)} />
      <StatBand d={d} />
      <div className="profile-grid">
        <Achievements d={d} />
        <aside className="profile-side">
          {d.categories.length > 0 && (
            <section className="panel panel-pad enter enter-3" aria-labelledby="pc-h">
              <div className="panel-head"><h2 id="pc-h">Category ranks</h2></div>
              <ul className="pcats">
                {d.categories.map((c) => (
                  <li key={c.key}>
                    <span className="pc-n"><Icon name={CATEGORY_ICON[c.key]} />{c.short}</span>
                    <Progress value={c.top ? (101 - c.top) / 100 : 0} />
                    <span className="pc-r num">{c.rank ? `#${c.rank}` : '—'}</span>
                  </li>
                ))}
              </ul>
            </section>
          )}
          <section className="panel panel-pad enter enter-4" aria-labelledby="bd-h">
            <div className="panel-head">
              <h2 id="bd-h">Badges</h2>
              <span className="pill">{d.badges.filter((b) => b.unlocked).length}/{d.badges.length}</span>
            </div>
            <ul className="badges">
              {d.badges.map((b) => (
                <li key={b.id} className={`badge-tile ${b.unlocked ? '' : 'locked'}`} title={b.unlocked ? 'Unlocked' : 'Locked'}>
                  <BadgeIcon glyph={b.glyph} color={b.unlocked ? '#B6F25A' : '#6F736B'} />
                  <b>{b.name}</b>
                  <small>{b.desc}</small>
                  <span className="sr-only">{b.unlocked ? 'Unlocked' : 'Locked'}</span>
                </li>
              ))}
            </ul>
          </section>
          <section className="panel panel-pad enter enter-5" aria-labelledby="sk-h">
            <div className="panel-head"><h2 id="sk-h">Skills</h2></div>
            {s.skills?.length ? (
              <div className="skill-cloud">{s.skills.map((k) => <span key={k} className="chip">{k}</span>)}</div>
            ) : (
              <p className="muted" style={{ fontWeight: 600 }}>{d.self ? 'Add skills from Edit profile.' : 'No skills listed.'}</p>
            )}
          </section>
        </aside>
      </div>

      {editing && <ProfileModal student={s} onClose={() => setEditing(false)} onSaved={(r) => r?.unlocked?.length && setUnlocked(r.unlocked)} />}
      {sharing && <ShareModal d={d} onClose={() => setSharing(false)} />}
      {unlocked[0] && <UnlockOverlay badge={unlocked[0]} onClose={() => setUnlocked((u) => u.slice(1))} />}
    </div>
  );
}

function PlayerCard({ d, onEdit, onShare }) {
  const s = d.student;
  const { openAdd } = useShell();
  const tier = s.tier;
  const links = [
    s.linkedin && { href: s.linkedin, label: 'LinkedIn', icon: 'linkedin' },
    s.github && { href: `https://github.com/${s.github}`, label: 'GitHub', icon: 'github' },
    s.leetcode && { href: `https://leetcode.com/u/${s.leetcode}`, label: 'LeetCode', icon: 'code' },
    s.codeforces && { href: `https://codeforces.com/profile/${s.codeforces}`, label: 'Codeforces', icon: 'trophy' },
    s.codechef && { href: `https://www.codechef.com/users/${s.codechef}`, label: 'CodeChef', icon: 'bolt' },
  ].filter(Boolean);
  return (
    <section className="panel player enter" style={{ '--tc': tier?.hex || '#8A8F98' }} aria-label="Profile summary">
      <div className="player-id">
        <div className="player-avatar">
          <Avatar student={s} size={112} />
        </div>
        <div className="player-text">
          <span className="label">{s.roll_no} · {s.course} {s.branch} · Batch of {s.grad_year}</span>
          <h1 className="display">{s.name}</h1>
          {s.headline && <p className="headline">{s.headline}</p>}
          {s.bio && <p className="bio">{s.bio}</p>}
          <div className="player-links">
            {links.map((l) => (
              <a key={l.label} href={l.href} target="_blank" rel="noreferrer noopener" className="link-chip">
                <Icon name={l.icon} /> {l.label}
              </a>
            ))}
            {d.self && links.length === 0 && <button className="link-chip" onClick={onEdit}><Icon name="plus" /> Add your links</button>}
          </div>
        </div>
      </div>
      <div className="player-stats">
        {s.rank !== null ? (
          <>
            <div className="ps-rank">
              <span className="label">College rank</span>
              <span className="rank-num"><i>#</i>{s.rank || '—'}</span>
              <span className="muted" style={{ fontSize: 12, fontWeight: 700 }}>{s.rank ? `of ${d.ranked} ranked` : 'Not ranked yet'}</span>
            </div>
            <div className="ps-tier">
              <TierHex tier={tier} size={54} />
              <div>
                <b className="tier-name" style={{ color: tier.color, fontSize: 28 }}>{tier.name}</b>
                <span className="mono ps-exp"><Count value={s.exp} /> EXP</span>
              </div>
            </div>
            {d.nextTier && (
              <div className="ps-next">
                <Progress value={(s.exp - tier.min) / (d.nextTier.min - tier.min)} color={tier.color} label={`Progress to ${d.nextTier.name}`} />
                <span>{fmt(d.nextTier.min - s.exp)} EXP to {d.nextTier.name}</span>
              </div>
            )}
          </>
        ) : (
          <div className="ps-rank"><span className="label">Rank</span><b className="muted">Hidden by the student</b></div>
        )}
        <div className="player-actions">
          {d.self ? (
            <>
              <button className="btn btn-primary" onClick={() => openAdd()}><Icon name="plus" strokeWidth={2.6} /> Add achievement</button>
              <button className="btn btn-ghost" onClick={onEdit}><Icon name="edit" /> Edit profile</button>
            </>
          ) : null}
          <button className="btn btn-ghost" onClick={onShare}><Icon name="share" /> {d.self ? 'Share card' : 'Profile card'}</button>
        </div>
      </div>
    </section>
  );
}

function StatBand({ d }) {
  const s = d.student;
  const approved = d.achievements.filter((a) => a.status === 'approved');
  const count = (k) => approved.filter((a) => a.category === k).length;
  const solved = (s.dsa_easy || 0) + (s.dsa_medium || 0) + (s.dsa_hard || 0);
  const parts = [
    ['easy', s.dsa_easy || 0, '#2ED3B7'],
    ['medium', s.dsa_medium || 0, '#FFC93C'],
    ['hard', s.dsa_hard || 0, '#FF6B4A'],
  ];
  return (
    <section className="stat-band enter enter-2" aria-label="Key stats">
      <div className="panel stat">
        <span className="label">Codeforces rating</span>
        <b className="display">{s.cf_rating ? <Count value={s.cf_rating} /> : '—'}</b>
        <small>{count('competitive')} approved contest{count('competitive') === 1 ? '' : 's'}</small>
      </div>
      <div className="panel stat wide">
        <span className="label">DSA problems solved</span>
        <b className="display"><Count value={solved} /></b>
        <div className="dsa-bar" role="img" aria-label={`Easy ${s.dsa_easy || 0}, medium ${s.dsa_medium || 0}, hard ${s.dsa_hard || 0}`}>
          {solved > 0 && parts.map(([k, v, c]) => <i key={k} style={{ flex: v, background: c }} />)}
        </div>
        <div className="dsa-legend">
          {parts.map(([k, v, c]) => <span key={k}><i style={{ background: c }} />{v} {k}</span>)}
        </div>
      </div>
      <div className="panel stat">
        <span className="label">Projects</span>
        <b className="display"><Count value={count('project')} /></b>
        <small>{s.github ? `github.com/${s.github}` : 'No GitHub linked'}</small>
      </div>
      <div className="panel stat">
        <span className="label">Experience</span>
        <b className="display"><Count value={count('internship')} /></b>
        <small>Approved internships</small>
      </div>
      <div className="panel stat">
        <span className="label">Certificates</span>
        <b className="display"><Count value={count('certification')} /></b>
        <small>Courses and exams</small>
      </div>
    </section>
  );
}

function Achievements({ d }) {
  const [cat, setCat] = useState('all');
  const [editing, setEditing] = useState(null);
  const [deleting, setDeleting] = useState(null);
  const { openAdd } = useShell();
  const { studentId } = useAuth();
  const toast = useToast();
  const items = d.achievements.filter((a) => cat === 'all' || a.category === cat);
  const tabs = [{ value: 'all', label: `All ${d.achievements.length}` }, ...CATEGORIES.map((c) => ({ value: c.key, label: c.short }))];

  async function confirmDelete() {
    try {
      await api.deleteAchievement(studentId, deleting.id);
      toast({ title: 'Achievement removed', body: deleting.title });
    } catch (e) {
      toast({ title: 'Could not remove it', body: e.message, tone: 'error' });
    }
    setDeleting(null);
  }

  return (
    <section className="panel panel-pad ach enter enter-3" id="achievements" aria-labelledby="ach-h">
      <div className="panel-head">
        <h2 id="ach-h">Achievements</h2>
        {d.self && <button className="btn btn-primary btn-sm" onClick={() => openAdd({ category: cat === 'all' ? undefined : cat })}><Icon name="plus" strokeWidth={2.6} /> Add</button>}
      </div>
      <Tabs items={tabs} value={cat} onChange={setCat} label="Filter achievements" />
      {d.achievementsHidden ? (
        <Empty icon="lock" title="Achievements are private">This student chose not to show their achievements.</Empty>
      ) : items.length === 0 ? (
        <Empty icon={CATEGORY_ICON[cat] || 'trophy'} title={cat === 'all' ? 'No achievements yet' : `No ${CATEGORY[cat].label.toLowerCase()} entries`} action={d.self && <button className="btn btn-ghost btn-sm" onClick={() => openAdd({ category: cat === 'all' ? undefined : cat })}>Add one</button>}>
          {d.self ? 'Add it with a proof link. A reviewer approves it and your EXP goes up.' : 'Nothing approved in this category yet.'}
        </Empty>
      ) : (
        <ul className="ach-list">
          {items.map((a) => (
            <li key={a.id} className="ach-item">
              <span className="ach-ic"><Icon name={CATEGORY_ICON[a.category]} /></span>
              <div className="ach-main">
                <div className="ach-title">
                  <b>{a.title}</b>
                  {d.self && <span className={`status status-${a.status}`}>{a.status}</span>}
                </div>
                <small>
                  {CATEGORY[a.category].label}
                  {a.organization ? ` · ${a.organization}` : ''}
                  {a.platform ? ` · ${a.platform}` : ''} · {shortDate(a.achieved_on)}
                </small>
                {a.description && <p>{a.description}</p>}
                {d.self && a.status === 'rejected' && a.review_note && <p className="note">Reviewer: {a.review_note}</p>}
              </div>
              <div className="ach-side">
                <span className="mono ach-pts">{a.score} pts</span>
                <div className="ach-actions">
                  <a className="icon-btn" href={a.proof_url} target="_blank" rel="noreferrer noopener" aria-label={`Open proof for ${a.title}`}><Icon name="link" /></a>
                  {d.self && a.status === 'pending' && (
                    <button className="icon-btn" onClick={() => setEditing(a)} aria-label={`Edit ${a.title}`}><Icon name="edit" /></button>
                  )}
                  {d.self && a.status !== 'approved' && (
                    <button className="icon-btn" onClick={() => setDeleting(a)} aria-label={`Remove ${a.title}`}><Icon name="trash" /></button>
                  )}
                </div>
              </div>
            </li>
          ))}
        </ul>
      )}
      {editing && <AchievementModal initial={editing} onClose={() => setEditing(null)} />}
      {deleting && (
        <Modal
          title="Remove this achievement?"
          subtitle={deleting.title}
          onClose={() => setDeleting(null)}
          footer={
            <>
              <button className="btn btn-ghost" onClick={() => setDeleting(null)} data-autofocus>Keep it</button>
              <button className="btn btn-danger" onClick={confirmDelete}>Remove</button>
            </>
          }
        >
          <div className="modal-body"><p className="muted" style={{ fontWeight: 600 }}>It leaves the review queue. You can add it again later.</p></div>
        </Modal>
      )}
    </section>
  );
}

function ShareModal({ d, onClose }) {
  const toast = useToast();
  const s = d.student;
  const url = `${window.location.origin}/app/students/${s.id}`;
  const top = d.achievements.filter((a) => a.status === 'approved').sort((a, b) => b.score - a.score).slice(0, 3);
  async function copy() {
    try {
      await navigator.clipboard.writeText(url);
      toast({ title: 'Link copied', body: 'Paste it anywhere to share your card.' });
    } catch {
      document.getElementById('share-url')?.select();
      toast({ title: 'Copy the link from the box', tone: 'info' });
    }
  }
  return (
    <Modal title="Profile card" subtitle="Share the link. It opens this student's profile." onClose={onClose}>
      <div className="modal-body">
        <div className="pcard" style={{ '--tc': s.tier?.hex || '#8A8F98' }}>
          <div className="pcard-top">
            <span className="brand-tile" style={{ width: 30, height: 30, fontSize: 16, borderRadius: 9 }}>GF</span>
            <GehuLogo height={20} />
          </div>
          <div className="pcard-id">
            <Avatar student={s} size={76} />
            <div>
              <b className="display">{s.name}</b>
              <small>{s.branch} · Batch of {s.grad_year} · {s.campus}</small>
            </div>
          </div>
          {s.rank !== null && (
            <div className="pcard-stats">
              <div><span className="label">Rank</span><b className="display">#{s.rank || '—'}</b></div>
              <div><span className="label">EXP</span><b className="display">{fmt(s.exp)}</b></div>
              <div><span className="label">Tier</span><b className="display" style={{ color: s.tier.color }}>{s.tier.name}</b></div>
            </div>
          )}
          {top.length > 0 && (
            <ul className="pcard-ach">
              {top.map((a) => <li key={a.id}><Icon name={CATEGORY_ICON[a.category]} /> {a.title}</li>)}
            </ul>
          )}
          {s.skills?.length > 0 && <div className="pcard-skills">{s.skills.slice(0, 5).map((k) => <span key={k}>{k}</span>)}</div>}
        </div>
        <div className="share-row">
          <label htmlFor="share-url" className="sr-only">Profile link</label>
          <input id="share-url" className="input mono" readOnly value={url} onFocus={(e) => e.target.select()} />
          <button className="btn btn-primary" onClick={copy}><Icon name="link" /> Copy link</button>
        </div>
      </div>
    </Modal>
  );
}

function ProfileSkeleton() {
  return (
    <div className="profile" aria-busy="true" aria-label="Loading profile">
      <div className="panel player"><div style={{ padding: 24, display: 'flex', gap: 20, alignItems: 'center', gridColumn: '1 / -1' }}><Skeleton h={112} w={112} r="50%" /><div style={{ flex: 1 }}><Skeleton h={14} w="40%" /><div style={{ height: 10 }} /><Skeleton h={40} w="60%" /></div></div></div>
      <div className="stat-band">{[0, 1, 2, 3, 4].map((i) => <div key={i} className="panel stat"><Skeleton h={60} /></div>)}</div>
    </div>
  );
}
