import { useState } from 'react';
import { Link, useLocation, useNavigate } from 'react-router-dom';
import { useAuth } from '../context/AuthContext.jsx';
import { getSession } from '../api/index.js';
import { useToast } from '../context/ToastContext.jsx';
import Icon from '../components/Icon.jsx';
import { BrandMark, GehuLogo, Field } from '../components/ui.jsx';
import { BRANCHES, CAMPUSES, GRAD_YEARS } from '../lib/validate.js';
import { useDocumentTitle } from '../lib/hooks.js';

export default function Auth({ mode }) {
  const signup = mode === 'signup';
  const { signIn, signInDemo, signUp } = useAuth();
  const toast = useToast();
  const navigate = useNavigate();
  const location = useLocation();
  const dest = location.state?.from || '/app';
  const goTo = (role) => (role === 'admin' ? '/app/review' : dest);
  const [form, setForm] = useState({ name: '', email: '', password: '', roll_no: '', branch: '', grad_year: '', campus: 'Dehradun' });
  const [errors, setErrors] = useState({});
  const [busy, setBusy] = useState(false);
  useDocumentTitle(signup ? 'Create your profile' : 'Sign in');
  const set = (k) => (e) => setForm((f) => ({ ...f, [k]: e.target.value }));

  async function run(fn, welcome) {
    setBusy(true);
    setErrors({});
    try {
      await fn();
      toast({ title: welcome });
      navigate(goTo(getSession()?.role), { replace: true });
    } catch (err) {
      if (err.errors) setErrors(err.errors);
      else toast({ title: 'Something went wrong', body: err.message, tone: 'error' });
    } finally {
      setBusy(false);
    }
  }

  const submit = (e) => {
    e.preventDefault();
    if (signup) run(() => signUp(form), 'Profile created. Add your first achievement.');
    else run(() => signIn({ email: form.email, password: form.password }), 'Welcome back');
  };
  const inv = (k) => ({ 'aria-invalid': errors[k] ? 'true' : undefined });

  return (
    <div className="auth">
      <aside className="auth-side">
        <div className="auth-brand">
          <BrandMark />
          <GehuLogo height={30} />
        </div>
        <div className="auth-pitch">
          <h1 className="display">{signup ? <>Make your<br /><span className="lime">player profile</span></> : <>Back to the<br /><span className="lime">college table</span></>}</h1>
          <p>Contests, DSA, projects, internships and certificates, verified and ranked against students at Graphic Era Hill University.</p>
        </div>
        <ul className="auth-points">
          <li><Icon name="check" strokeWidth={3} /> One profile for your whole technical record</li>
          <li><Icon name="check" strokeWidth={3} /> Every entry checked by a reviewer</li>
          <li><Icon name="check" strokeWidth={3} /> See exactly who to pass next</li>
        </ul>
      </aside>

      <main className="auth-main" id="main">
        <form className="auth-form" onSubmit={submit} noValidate>
          <div className="auth-switch" role="tablist" aria-label="Account">
            <Link to="/login" role="tab" aria-selected={!signup} className={!signup ? 'on' : ''} state={location.state}>Sign in</Link>
            <Link to="/signup" role="tab" aria-selected={signup} className={signup ? 'on' : ''} state={location.state}>Create account</Link>
          </div>
          <h2>{signup ? 'Create your profile' : 'Sign in to Forge'}</h2>

          {signup && (
            <Field id="su-name" label="Full name" error={errors.name}>
              <input id="su-name" className="input" autoComplete="name" value={form.name} onChange={set('name')} {...inv('name')} />
            </Field>
          )}
          <Field id="su-email" label="Email" error={errors.email} hint={signup ? 'Your university email works best' : undefined}>
            <input id="su-email" type="email" className="input" autoComplete="email" value={form.email} onChange={set('email')} {...inv('email')} />
          </Field>
          <Field id="su-password" label="Password" error={errors.password} hint={signup ? 'At least 8 characters' : undefined}>
            <input id="su-password" type="password" className="input" autoComplete={signup ? 'new-password' : 'current-password'} value={form.password} onChange={set('password')} {...inv('password')} />
          </Field>
          {signup && (
            <div className="form-grid">
              <Field id="su-roll" label="University roll no." error={errors.roll_no} hint="10 digits, e.g. 2512510044">
                <input id="su-roll" className="input mono" inputMode="numeric" maxLength={10} value={form.roll_no} onChange={(e) => setForm((f) => ({ ...f, roll_no: e.target.value.replace(/\D/g, '') }))} {...inv('roll_no')} />
              </Field>
              <Field id="su-branch" label="Branch" error={errors.branch}>
                <select id="su-branch" className="select" value={form.branch} onChange={set('branch')} {...inv('branch')}>
                  <option value="">Choose…</option>
                  {BRANCHES.map((b) => <option key={b}>{b}</option>)}
                </select>
              </Field>
              <Field id="su-batch" label="Batch (graduating)" error={errors.grad_year}>
                <select id="su-batch" className="select" value={form.grad_year} onChange={set('grad_year')} {...inv('grad_year')}>
                  <option value="">Choose…</option>
                  {GRAD_YEARS.map((y) => <option key={y}>{y}</option>)}
                </select>
              </Field>
              <Field id="su-campus" label="Campus">
                <select id="su-campus" className="select" value={form.campus} onChange={set('campus')}>
                  {CAMPUSES.map((c) => <option key={c}>{c}</option>)}
                </select>
              </Field>
            </div>
          )}

          <button type="submit" className="btn btn-primary btn-lg btn-block" disabled={busy}>
            {busy ? 'Please wait…' : signup ? 'Create profile' : 'Sign in'}
          </button>

          <div className="auth-or"><span>or</span></div>
          <div className="auth-demo">
            <button type="button" className="btn btn-ghost" disabled={busy} onClick={() => run(() => signInDemo('student'), 'Signed in to the demo student')}>
              <Icon name="bolt" /> Demo student
            </button>
            <button type="button" className="btn btn-ghost" disabled={busy} onClick={() => run(() => signInDemo('reviewer'), 'Signed in as the demo reviewer')}>
              <Icon name="check" /> Demo reviewer
            </button>
          </div>
          <p className="auth-note">Demo buttons are for the project evaluation and can be switched off on the server.</p>
          <Link to="/" className="auth-back"><Icon name="arrow" /> Back to the home page</Link>
        </form>
      </main>
    </div>
  );
}
