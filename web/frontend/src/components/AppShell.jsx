import { createContext, useContext, useState } from 'react';
import { NavLink, Outlet, useNavigate, useLocation, Navigate } from 'react-router-dom';
import Icon from './Icon.jsx';
import { BrandMark, GehuLogo, Avatar } from './ui.jsx';
import AchievementModal from './AchievementModal.jsx';
import { useAuth } from '../context/AuthContext.jsx';

const ShellContext = createContext({ openAdd: () => {} });
export const useShell = () => useContext(ShellContext);

const STUDENT_NAV = [
  { to: '/app', label: 'Dashboard', icon: 'home', end: true },
  { to: '/app/search', label: 'Leaderboard', icon: 'trophy' },
  { to: '/app/profile', label: 'Profile', icon: 'user' },
];
// Reviewer ka apna menu: queue + leaderboard
const REVIEWER_NAV = [
  { to: '/app/review', label: 'Review queue', icon: 'check' },
  { to: '/app/search', label: 'Leaderboard', icon: 'trophy' },
];

export function RequireAuth({ children, role }) {
  const { session } = useAuth();
  const loc = useLocation();
  if (!session) return <Navigate to="/login" replace state={{ from: loc.pathname + loc.search }} />;
  if (role && session.role !== role) return <Navigate to={session.role === 'admin' ? '/app/review' : '/app'} replace />;
  return children;
}

export function HomeRoute({ children }) {
  const { session } = useAuth();
  return session?.role === 'admin' ? <Navigate to="/app/review" replace /> : children;
}

export default function AppShell() {
  const { me, role, signOut } = useAuth();
  const reviewer = role === 'admin';
  const NAV = reviewer ? REVIEWER_NAV : STUDENT_NAV;
  const navigate = useNavigate();
  const [adding, setAdding] = useState(null);
  const [q, setQ] = useState('');

  const openAdd = (initial = {}) => setAdding(initial);

  function onSearch(e) {
    e.preventDefault();
    navigate(`/app/search${q.trim() ? `?q=${encodeURIComponent(q.trim())}` : ''}`);
  }

  return (
    <ShellContext.Provider value={{ openAdd }}>
      <a href="#main" className="skip-link">Skip to content</a>
      <div className="shell">
        <aside className="sidebar" aria-label="Main">
          <BrandMark to="/app" compact />
          <nav className="side-nav">
            {NAV.map((n) => (
              <NavLink key={n.to} to={n.to} end={n.end} className="side-link" aria-label={n.label}>
                <Icon name={n.icon} />
                <span className="side-tip">{n.label}</span>
              </NavLink>
            ))}
            {!reviewer && (
              <button className="side-link add" onClick={() => openAdd()} aria-label="Add achievement">
                <Icon name="plus" strokeWidth={2.6} />
                <span className="side-tip">Add achievement</span>
              </button>
            )}
          </nav>
          <div className="side-foot">
            <button className="side-link" onClick={() => { signOut(); navigate('/'); }} aria-label="Sign out">
              <Icon name="logout" />
              <span className="side-tip">Sign out</span>
            </button>
          </div>
        </aside>

        <div className="shell-main">
          <header className="topbar">
            <div className="topbar-left">
              <BrandMark to="/app" />
              <span className="topbar-uni">
                <GehuLogo height={26} />
              </span>
            </div>
            <form className="search" role="search" onSubmit={onSearch}>
              <Icon name="search" />
              <label htmlFor="top-search" className="sr-only">Search students and skills</label>
              <input id="top-search" value={q} onChange={(e) => setQ(e.target.value)} placeholder="Search students, skills, roll no…" />
            </form>
            <div className="topbar-right">
              {reviewer ? (
                <span className="pill reviewer-pill"><Icon name="check" size={14} strokeWidth={2.8} /> Reviewer</span>
              ) : (
                <button className="btn btn-primary btn-sm hide-sm" onClick={() => openAdd()}>
                  <Icon name="plus" strokeWidth={2.6} /> Add achievement
                </button>
              )}
              <NavLink to={reviewer ? '/app/review' : '/app/profile'} className="me-link" aria-label={reviewer ? 'Review queue' : 'Your profile'}>
                {me ? <Avatar student={me} size={38} /> : <span className="skeleton" style={{ width: 38, height: 38, borderRadius: '50%' }} />}
              </NavLink>
            </div>
          </header>
          <main id="main" className="page" tabIndex={-1}>
            <Outlet />
          </main>
        </div>

        <nav className={`bottom-nav ${reviewer ? 'three' : ''}`} aria-label="Main">
          {NAV.slice(0, 2).map((n) => (
            <NavLink key={n.to} to={n.to} end={n.end} className="bn-link">
              <Icon name={n.icon} />
              <span>{n.label}</span>
            </NavLink>
          ))}
          {!reviewer && (
            <>
              <button className="bn-add" onClick={() => openAdd()} aria-label="Add achievement">
                <Icon name="plus" strokeWidth={2.8} />
              </button>
              <NavLink to="/app/profile" className="bn-link">
                <Icon name="user" />
                <span>Profile</span>
              </NavLink>
            </>
          )}
          <button className="bn-link" onClick={() => { signOut(); navigate('/'); }}>
            <Icon name="logout" />
            <span>Sign out</span>
          </button>
        </nav>
      </div>
      {adding && <AchievementModal initial={adding} onClose={() => setAdding(null)} />}
    </ShellContext.Provider>
  );
}
