import { useEffect, useLayoutEffect, useRef, useState } from 'react';
import { Link } from 'react-router-dom';
import Icon from './Icon.jsx';
import { initials, fmt } from '../lib/format.js';
import { useCountUp, useMounted } from '../lib/hooks.js';

export function BrandMark({ to = '/', compact = false }) {
  return (
    <Link to={to} className="brand" aria-label="GEHU Forge home">
      <span className="brand-tile">GF</span>
      {!compact && <span className="brand-word">FORGE</span>}
    </Link>
  );
}

export function GehuLogo({ className = '', height = 34 }) {
  return <img src="/gehu-logo-white.svg" alt="Graphic Era Hill University" className={`gehu-logo ${className}`} style={{ height }} width={height * (244 / 59)} height={height} />;
}

export function Avatar({ student, size = 36, ring }) {
  const style = { width: size, height: size, fontSize: Math.round(size * 0.36), '--ring': ring || student?.tier?.color || 'var(--tier-candidate)' };
  return (
    <span className="avatar" style={style} aria-hidden="true">
      {student?.avatar_url ? <img src={student.avatar_url} alt="" /> : initials(student?.name)}
    </span>
  );
}

export function TierHex({ tier, size = 62, letter }) {
  const c = tier?.hex || '#8A8F98';
  return (
    <svg width={size} height={size * 1.1} viewBox="0 0 62 68" aria-hidden="true" className="tier-hex">
      <path d="M31 3l26 15v32L31 65 5 50V18z" fill={`${c}24`} stroke={c} strokeWidth="3" />
      <path d="M31 15l15 8.5v19L31 51l-15-8.5v-19z" fill={c} />
      <text x="31" y="40" textAnchor="middle" fontFamily="Barlow Condensed, Arial Narrow, sans-serif" fontWeight="800" fontSize="20" fill="#0E0F0C">
        {letter || tier?.name?.[0] || 'C'}
      </text>
    </svg>
  );
}

export function TierTag({ tier }) {
  if (!tier) return null;
  return (
    <span className="tier-tag" style={{ color: tier.color }}>
      <i aria-hidden="true" />
      {tier.name}
    </span>
  );
}

export function MoveTag({ move, compact = false }) {
  if (!move) return <span className="mv">—</span>;
  if (move.kind === 'new') return <span className="mv up">NEW</span>;
  if (move.value > 0) return <span className="mv up" aria-label={`Up ${move.value} places`}>▲{move.value}{compact ? '' : ''}</span>;
  if (move.value < 0) return <span className="mv down" aria-label={`Down ${-move.value} places`}>▼{-move.value}</span>;
  return <span className="mv down" aria-label="No change">–</span>;
}

export function Progress({ value, color, label }) {
  const on = useMounted();
  return (
    <div className="track" role={label ? 'progressbar' : undefined} aria-label={label} aria-valuenow={label ? Math.round(value * 100) : undefined} aria-valuemin={label ? 0 : undefined} aria-valuemax={label ? 100 : undefined}>
      <div className="fill" style={{ width: on ? `${Math.max(0, Math.min(1, value)) * 100}%` : 0, background: color }} />
    </div>
  );
}

export function Count({ value, decimals = 0 }) {
  const v = useCountUp(value);
  return <span className="num">{decimals ? v.toFixed(decimals) : fmt(v)}</span>;
}

export function ExpDigits({ value }) {
  const v = useCountUp(value, { duration: 1100 });
  const s = Math.round(v).toLocaleString('en-US').padStart(5, '0');
  return (
    <span className="digits" aria-label={`${fmt(value)} EXP`}>
      {s.split('').map((ch, i) =>
        ch === ',' ? <span key={i} className="comma" aria-hidden="true">,</span> : <span key={i} className="dg" aria-hidden="true">{ch}</span>
      )}
    </span>
  );
}

export function RankNumber({ rank, className = '' }) {
  const v = useCountUp(rank || 0, { duration: 900, from: rank ? Math.min(250, rank + 36) : 0 });
  if (!rank) return <span className={`rank-num none ${className}`} aria-label="Not ranked">#–</span>;
  return (
    <span className={`rank-num ${className}`} aria-label={`Rank ${rank}`}>
      <i aria-hidden="true">#</i>
      <span aria-hidden="true">{Math.round(v)}</span>
    </span>
  );
}

export function Tabs({ items, value, onChange, label, size }) {
  const ref = useRef(null);
  const [ind, setInd] = useState(null);
  useLayoutEffect(() => {
    const el = ref.current?.querySelector('[aria-selected="true"]');
    if (el) setInd({ left: el.offsetLeft, width: el.offsetWidth, top: el.offsetTop, height: el.offsetHeight });
  }, [value, items]);
  useEffect(() => {
    const on = () => {
      const el = ref.current?.querySelector('[aria-selected="true"]');
      if (el) setInd({ left: el.offsetLeft, width: el.offsetWidth, top: el.offsetTop, height: el.offsetHeight });
    };
    window.addEventListener('resize', on);
    document.fonts?.ready.then(on);
    return () => window.removeEventListener('resize', on);
  }, []);
  const onKey = (e) => {
    const i = items.findIndex((x) => x.value === value);
    if (e.key === 'ArrowRight' || e.key === 'ArrowLeft') {
      e.preventDefault();
      const n = items[(i + (e.key === 'ArrowRight' ? 1 : items.length - 1)) % items.length];
      onChange(n.value);
      requestAnimationFrame(() => ref.current?.querySelector('[aria-selected="true"]')?.focus());
    }
  };
  return (
    <div className={`tabs ${size || ''}`} role="tablist" aria-label={label} ref={ref} onKeyDown={onKey}>
      {ind && <span className="tab-indicator" style={ind} aria-hidden="true" />}
      {items.map((it) => (
        <button
          key={it.value}
          type="button"
          role="tab"
          className="tab"
          aria-selected={it.value === value}
          tabIndex={it.value === value ? 0 : -1}
          onClick={() => onChange(it.value)}
        >
          {it.label}
        </button>
      ))}
    </div>
  );
}

export function Modal({ title, subtitle, onClose, children, footer, labelledBy = 'modal-title', wide }) {
  const ref = useRef(null);
  useEffect(() => {
    const prev = document.activeElement;
    const el = ref.current;
    const focusables = () => el.querySelectorAll('button, [href], input, select, textarea, [tabindex]:not([tabindex="-1"])');
    (el.querySelector('[data-autofocus]') || focusables()[0])?.focus();
    const onKey = (e) => {
      if (e.key === 'Escape') onClose();
      if (e.key === 'Tab') {
        const f = [...focusables()].filter((x) => !x.disabled);
        if (!f.length) return;
        if (e.shiftKey && document.activeElement === f[0]) { e.preventDefault(); f[f.length - 1].focus(); }
        else if (!e.shiftKey && document.activeElement === f[f.length - 1]) { e.preventDefault(); f[0].focus(); }
      }
    };
    document.addEventListener('keydown', onKey);
    document.body.style.overflow = 'hidden';
    return () => {
      document.removeEventListener('keydown', onKey);
      document.body.style.overflow = '';
      prev?.focus?.();
    };
  }, [onClose]);
  return (
    <div className="modal-backdrop" onMouseDown={(e) => e.target === e.currentTarget && onClose()}>
      <div className="modal" role="dialog" aria-modal="true" aria-labelledby={labelledBy} ref={ref} style={wide ? { width: 'min(760px, 100%)' } : undefined}>
        <div className="modal-head">
          <div>
            <h2 id={labelledBy}>{title}</h2>
            {subtitle && <p>{subtitle}</p>}
          </div>
          <button className="icon-btn" onClick={onClose} aria-label="Close">
            <Icon name="x" />
          </button>
        </div>
        {children}
        {footer && <div className="modal-foot">{footer}</div>}
      </div>
    </div>
  );
}

export function Field({ id, label, hint, error, children }) {
  return (
    <div className="field">
      <label htmlFor={id}>{label}</label>
      {children}
      {error ? <span className="error" id={`${id}-error`}>{error}</span> : hint ? <span className="hint" id={`${id}-hint`}>{hint}</span> : null}
    </div>
  );
}

export function Switch({ id, checked, onChange, label, detail }) {
  return (
    <div className="toggle">
      <label htmlFor={id}>
        <b>{label}</b>
        {detail && <small>{detail}</small>}
      </label>
      <span className="switch">
        <input id={id} type="checkbox" role="switch" checked={checked} onChange={(e) => onChange(e.target.checked)} />
        <span />
      </span>
    </div>
  );
}

export function Skeleton({ h = 16, w = '100%', r }) {
  return <div className="skeleton" style={{ height: h, width: w, borderRadius: r }} aria-hidden="true" />;
}

export function Empty({ icon = 'info', title, children, action }) {
  return (
    <div className="empty">
      <Icon name={icon} />
      <b>{title}</b>
      {children && <p>{children}</p>}
      {action}
    </div>
  );
}

export function ErrorState({ error, onRetry }) {
  return (
    <Empty icon="info" title="Something went wrong" action={onRetry && <button className="btn btn-ghost btn-sm" onClick={onRetry}>Try again</button>}>
      {error?.message || 'The data could not load.'}
    </Empty>
  );
}

const GLYPHS = {
  check: <path d="M47 61l9 9 17-19" stroke="#1B2608" strokeWidth="7" strokeLinecap="round" strokeLinejoin="round" fill="none" />,
  up: <path d="M61 42l12 18h-7v16h-10V60h-7z" fill="#1B2608" />,
  crown: <path d="M45 72l-3-20 10 8 9-13 9 13 10-8-3 20z" fill="#1B2608" />,
  star: <path d="M61 43l5 11 12 1-9 8 3 12-11-6-11 6 3-12-9-8 12-1z" fill="#1B2608" />,
  flame: <path d="M61 42c2 9 12 11 12 22a12 12 0 0 1-24 0c0-6 3-9 5-13 1 4 3 6 5 7-1-6 0-11 2-16z" fill="#1B2608" />,
  user: <path d="M61 59a8 8 0 1 0 0-16 8 8 0 0 0 0 16zM47 78a14 14 0 0 1 28 0z" fill="#1B2608" />,
  grid: <path d="M47 47h12v12H47zM63 47h12v12H63zM47 63h12v12H47zM63 63h12v12H63z" fill="#1B2608" />,
  case: <path d="M46 54h30v20H46zM55 54v-5h12v5" stroke="#1B2608" strokeWidth="5" fill="none" strokeLinejoin="round" />,
};

export function BadgeIcon({ glyph = 'check', color = '#B6F25A', size = 54 }) {
  return (
    <svg viewBox="0 0 122 122" width={size} height={size} aria-hidden="true">
      <path d="M61 4l50 28.5v57L61 118 11 89.5v-57z" fill="#1D1F23" stroke={color} strokeWidth="4" />
      <path d="M61 24l33 19v36L61 98 28 79V43z" fill={color} />
      {GLYPHS[glyph]}
    </svg>
  );
}

export function UnlockOverlay({ badge, onClose }) {
  return (
    <div className="modal-backdrop" onMouseDown={(e) => e.target === e.currentTarget && onClose()}>
      <div className="unlock" role="dialog" aria-modal="true" aria-labelledby="unlock-title">
        <div className="burst">
          <span className="ring" />
          <span className="ring two" />
          <BadgeIcon glyph={badge.glyph} size={122} />
        </div>
        <div className="lab">BADGE UNLOCKED</div>
        <h3 id="unlock-title">{badge.name}</h3>
        <p>{badge.desc}</p>
        <button className="btn btn-primary btn-block" onClick={onClose} autoFocus>
          Nice
        </button>
      </div>
    </div>
  );
}

export function StudentPreview({ row, style }) {
  if (!row) return null;
  return (
    <div className="preview" style={style} role="tooltip">
      <div className="p-top">
        <Avatar student={row} size={44} />
        <div>
          <b>{row.name}</b>
          <small>{row.branch} · Batch of {row.grad_year} · {row.campus}</small>
        </div>
      </div>
      {row.headline && <p className="muted" style={{ fontSize: 12.5, fontWeight: 600, marginTop: 10 }}>{row.headline}</p>}
      <div className="p-stats">
        <div><span className="label">Rank</span><b>{row.rank ? `#${row.rank}` : '—'}</b></div>
        <div><span className="label">EXP</span><b>{fmt(row.exp)}</b></div>
        <div><span className="label">Tier</span><b style={{ color: row.tier.color, fontSize: 15, paddingTop: 4 }}>{row.tier.name}</b></div>
      </div>
      {row.skills?.length > 0 && (
        <div className="p-skills">
          {row.skills.slice(0, 5).map((s) => <span key={s}>{s}</span>)}
        </div>
      )}
    </div>
  );
}
