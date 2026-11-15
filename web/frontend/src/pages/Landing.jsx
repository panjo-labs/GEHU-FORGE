import { useEffect, useState } from 'react';
import { Link } from 'react-router-dom';
import * as api from '../api/index.js';
import { useAuth } from '../context/AuthContext.jsx';
import Icon, { CATEGORY_ICON } from '../components/Icon.jsx';
import { BrandMark, GehuLogo, Avatar, TierTag, TierHex, Count, MoveTag, Skeleton } from '../components/ui.jsx';
import { CATEGORIES, TIERS } from '../lib/scoring.js';
import { fmt } from '../lib/format.js';
import { useDocumentTitle } from '../lib/hooks.js';

const EXAMPLES = {
  competitive: 'Codeforces, CodeChef, LeetCode and HackerRank contests',
  dsa: 'Problems solved and topic sheets finished',
  project: 'Repos, merged pull requests and things you shipped',
  internship: 'Internships and work of any kind',
  certification: 'NPTEL, Coursera and cloud certificates',
  academic: 'Hackathons, papers, clubs and toppers',
};

export default function Landing() {
  const { session } = useAuth();
  const [d, setD] = useState(null);
  const [error, setError] = useState(false);
  useDocumentTitle('');

  useEffect(() => {
    api.getLanding().then(setD).catch(() => setError(true));
  }, []);

  const primary = session ? { to: '/app', label: 'Open your dashboard' } : { to: '/signup', label: 'Create your profile' };

  return (
    <div className="landing">
      <a href="#hero" className="skip-link">Skip to content</a>
      <header className="l-nav">
        <div className="l-wrap l-nav-in">
          <div className="l-brand">
            <BrandMark />
            <span className="l-divider" aria-hidden="true" />
            <GehuLogo height={30} />
          </div>
          <nav className="l-links" aria-label="Sections">
            <a href="#how">How it works</a>
            <a href="#categories">Categories</a>
            <a href="#tiers">Tiers</a>
            <a href="#leaderboard">Leaderboard</a>
          </nav>
          <div className="l-cta">
            {!session && <Link to="/login" className="btn btn-quiet">Sign in</Link>}
            <Link to={primary.to} className="btn btn-primary btn-sm">{primary.label}</Link>
          </div>
        </div>
      </header>

      <main>
        <section className="l-hero" id="hero">
          <div className="l-wrap hero-grid">
            <div className="hero-copy enter">
              <span className="eyebrow"><span className="dot-lime" aria-hidden="true" /> For Graphic Era Hill University students</span>
              <h1 className="display">
                <span className="lime">Rank up</span> at<br />Graphic Era Hill
              </h1>
              <p className="lead">
                Forge turns your contests, DSA, projects, internships and certificates into one verified score, and shows exactly where you stand among {d ? fmt(d.students) : 'your'} students on campus.
              </p>
              <div className="hero-actions">
                <Link to={primary.to} className="btn btn-primary btn-lg">{primary.label} <Icon name="arrow" /></Link>
                <a href="#how" className="btn btn-ghost btn-lg">See how scoring works</a>
              </div>
              <ul className="hero-facts">
                <li><b className="num">{d ? <Count value={d.verified} /> : '—'}</b> verified achievements</li>
                <li><b>6</b> ranking categories</li>
                <li><b>5</b> tiers to climb</li>
              </ul>
            </div>
            <HeroBoard d={d} error={error} />
          </div>
        </section>

        <section className="l-section" id="how" aria-labelledby="how-h">
          <div className="l-wrap">
            <div className="l-head">
              <span className="label">How it works</span>
              <h2 id="how-h" className="display">Three steps from proof to rank</h2>
            </div>
            <ol className="steps">
              <li className="step">
                <span className="step-n display">01</span>
                <h3>Add what you did</h3>
                <p>A certificate, a contest standing, a merged pull request or an internship, with a proof link.</p>
                <div className="frag frag-form" aria-hidden="true">
                  <span className="ff-label">Title</span>
                  <span className="ff-input">Cloud Computing, NPTEL Elite</span>
                  <span className="ff-label">Proof link</span>
                  <span className="ff-input mono">https://nptel.ac.in/…</span>
                </div>
              </li>
              <li className="step">
                <span className="step-n display">02</span>
                <h3>A reviewer verifies it</h3>
                <p>Every entry waits in a first-come review queue. Only approved ones count, and each counts once.</p>
                <div className="frag frag-review" aria-hidden="true">
                  <span className="status status-pending">pending</span>
                  <Icon name="arrow" />
                  <span className="status status-approved">approved</span>
                  <span className="mono lime">+240 EXP</span>
                </div>
              </li>
              <li className="step">
                <span className="step-n display">03</span>
                <h3>Climb the college table</h3>
                <p>Your EXP updates, your rank moves, and the dashboard shows the next person to pass.</p>
                <div className="frag frag-rank" aria-hidden="true">
                  <span className="display big">#11</span>
                  <span className="pill delta-up">▲ 4 this week</span>
                </div>
              </li>
            </ol>
          </div>
        </section>

        <section className="l-section alt" id="categories" aria-labelledby="cat-h">
          <div className="l-wrap">
            <div className="l-head">
              <span className="label">Six categories</span>
              <h2 id="cat-h" className="display">Not all wins weigh the same</h2>
              <p>Points are multiplied by how much the work says about you. An internship counts five times a short course. Contest points also scale by platform.</p>
            </div>
            <div className="bento">
              {CATEGORIES.slice().sort((a, b) => b.weight - a.weight).map((c, i) => (
                <article key={c.key} className={`bento-tile b-${i}`}>
                  <div className="bt-top">
                    <span className="bt-ic"><Icon name={CATEGORY_ICON[c.key]} /></span>
                    <span className="bt-w display">×{c.weight.toFixed(1)}</span>
                  </div>
                  <h3>{c.label}</h3>
                  <p>{EXAMPLES[c.key]}</p>
                  <div className="bt-bar" aria-hidden="true"><i style={{ width: `${(c.weight / 5) * 100}%` }} /></div>
                </article>
              ))}
            </div>
          </div>
        </section>

        <section className="l-section" id="tiers" aria-labelledby="tier-h">
          <div className="l-wrap">
            <div className="l-head">
              <span className="label">Five tiers</span>
              <h2 id="tier-h" className="display">From Candidate to Grandmaster</h2>
              <p>EXP runs from 0 to 10,000 on a log scale, so early wins move you fast and the top takes real work.</p>
            </div>
            <ol className="ladder">
              {TIERS.map((t, i) => (
                <li key={t.name} style={{ '--tc': t.hex }}>
                  <TierHex tier={t} size={58} />
                  <b className="tier-name" style={{ color: t.color }}>{t.name}</b>
                  <span className="mono">{fmt(t.min)}+ EXP</span>
                  <span className="ladder-step" style={{ height: 24 + i * 22 }} aria-hidden="true" />
                </li>
              ))}
            </ol>
          </div>
        </section>

        <section className="l-section alt" id="leaderboard" aria-labelledby="lb-h">
          <div className="l-wrap lb-preview">
            <div className="l-head">
              <span className="label">Live table</span>
              <h2 id="lb-h" className="display">This week at the top</h2>
              <p>Rank movement compares with the table seven days ago.</p>
              <Link to={session ? '/app/search' : '/login'} className="btn btn-ghost">{session ? 'Open the full leaderboard' : 'Sign in to find your rank'} <Icon name="arrow" /></Link>
            </div>
            <div className="panel lb-card">
              {!d && !error && Array.from({ length: 6 }, (_, i) => <div key={i} style={{ padding: 10 }}><Skeleton h={30} /></div>)}
              {error && <p className="muted" style={{ padding: 16 }}>The table could not load. Refresh to try again.</p>}
              {d && (
                <ol className="lb-full">
                  {d.top.map((r) => (
                    <li key={r.id} className={`lb-row ${r.rank <= 3 ? 'top' : ''}`}>
                      <span className="rk">{r.rank}</span>
                      <Avatar student={r} size={34} />
                      <span className="who"><b>{r.name}</b><small>{r.branch} · {r.grad_year} · {r.campus}</small></span>
                      <span className="hide-xs"><TierTag tier={r.tier} /></span>
                      <span className="hide-xs"><MoveTag move={r.move} /></span>
                      <span className="xp">{fmt(r.exp)}</span>
                    </li>
                  ))}
                </ol>
              )}
            </div>
          </div>
        </section>

        <section className="l-final">
          <div className="l-wrap final-in">
            <h2 className="display">Your rank is waiting</h2>
            <p>Make your profile in two minutes. Add one achievement and you are on the board as soon as it is approved.</p>
            <Link to={primary.to} className="btn btn-dark btn-lg">{primary.label} <Icon name="arrow" /></Link>
          </div>
        </section>
      </main>

      <footer className="l-footer">
        <div className="l-wrap foot-in">
          <div className="foot-brand">
            <BrandMark />
            <GehuLogo height={34} />
          </div>
          <p>Built by Pranjal Saraswat, Divyanshi Purohit, Saksham Singh and Manya Singh, B.Tech CSE, Graphic Era Hill University, Dehradun. Project-based learning, 2026.</p>
          <p className="foot-note">The students and achievements shown are generated demo data, not real people. Graphic Era Hill University and its logo belong to the university.</p>
        </div>
      </footer>
    </div>
  );
}

function HeroBoard({ d, error }) {
  if (error) return <div className="hero-board panel"><p className="muted" style={{ padding: 20 }}>The live table could not load.</p></div>;
  const rows = d?.top.slice(0, 5) || [];
  return (
    <div className="hero-visual enter enter-2" aria-label="Preview of the college leaderboard">
      <div className="hero-board panel">
        <div className="hb-head">
          <span className="label">College leaderboard · Overall</span>
          <span className="pill">{d ? `${d.ranked} ranked` : '…'}</span>
        </div>
        <ol className="lb-full">
          {rows.length === 0 && Array.from({ length: 5 }, (_, i) => <li key={i} style={{ padding: 10 }}><Skeleton h={30} /></li>)}
          {rows.map((r, i) => (
            <li key={r.id} className={`lb-row ${i < 3 ? 'top' : ''}`} style={{ animation: `enter .5s var(--ease-out) ${0.3 + i * 0.07}s both` }}>
              <span className="rk">{r.rank}</span>
              <Avatar student={r} size={34} />
              <span className="who"><b>{r.name}</b><small>{r.branch} · {r.grad_year}</small></span>
              <span className="hide-xs"><TierTag tier={r.tier} /></span>
              <span className="hide-xs" />
              <span className="xp">{fmt(r.exp)}</span>
            </li>
          ))}
          <li className="lb-gap" aria-hidden="true">· · ·</li>
          <li className="lb-row you hero-you">
            <span className="rk">11</span>
            <span className="avatar" style={{ width: 34, height: 34, fontSize: 12, '--ring': 'var(--tier-expert)' }}>YOU</span>
            <span className="who"><b>Your name here</b><small>Your branch · your batch</small></span>
            <span className="hide-xs"><TierTag tier={TIERS[2]} /></span>
            <span className="mv up hide-xs">▲4</span>
            <span className="xp">5,230</span>
          </li>
        </ol>
      </div>
      <div className="hero-toast" aria-hidden="true">
        <span className="ok"><Icon name="check" strokeWidth={3.2} /></span>
        <div><b>Achievement approved</b><span>Codeforces Round 958 · <em className="mono lime">+240 EXP</em></span></div>
      </div>
    </div>
  );
}
