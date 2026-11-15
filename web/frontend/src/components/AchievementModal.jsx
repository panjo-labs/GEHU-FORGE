import { useState } from 'react';
import Icon, { CATEGORY_ICON } from './Icon.jsx';
import { Modal, Field } from './ui.jsx';
import { CATEGORIES, CATEGORY, PLATFORMS } from '../lib/scoring.js';
import * as api from '../api/index.js';
import { useAuth } from '../context/AuthContext.jsx';
import { useToast } from '../context/ToastContext.jsx';

const ORG_LABEL = {
  competitive: ['Contest', 'e.g. Codeforces Round 958 (Div. 2)'],
  dsa: ['Sheet or source', 'e.g. LeetCode, Striver sheet'],
  project: ['Repository or team', 'e.g. GitHub, open source org'],
  internship: ['Company', 'e.g. Nimbus Labs'],
  certification: ['Issued by', 'e.g. NPTEL, Coursera'],
  academic: ['Awarded by', 'e.g. Department of CSE'],
};
const TITLE_HINT = {
  competitive: 'Rank 312 in Codeforces Round 958',
  dsa: '250 problems solved',
  project: 'Campus events app with React and FastAPI',
  internship: 'Backend intern, 8 weeks',
  certification: 'Cloud Computing (NPTEL, Elite)',
  academic: 'Inter-college hackathon, 2nd place',
};

const today = () => new Date().toISOString().slice(0, 10);

export default function AchievementModal({ initial, onClose, onSaved }) {
  const { studentId } = useAuth();
  const toast = useToast();
  const editing = Boolean(initial?.id);
  const [form, setForm] = useState(() => ({
    category: initial?.category || 'certification',
    title: initial?.title || '',
    organization: initial?.organization || '',
    achieved_on: initial?.achieved_on || today(),
    score: initial?.score ?? '',
    platform: initial?.platform || '',
    proof_url: initial?.proof_url || '',
    description: initial?.description || '',
  }));
  const [errors, setErrors] = useState({});
  const [busy, setBusy] = useState(false);
  const set = (k) => (e) => setForm((f) => ({ ...f, [k]: e?.target ? e.target.value : e }));
  const cat = CATEGORY[form.category];
  const usesPlatform = form.category === 'competitive' || form.category === 'dsa';

  async function submit(e) {
    e.preventDefault();
    setBusy(true);
    setErrors({});
    try {
      const payload = { ...form, score: form.score === '' ? '' : Number(form.score), platform: usesPlatform ? form.platform : '' };
      if (editing) await api.updateAchievement(studentId, initial.id, payload);
      else await api.addAchievement(studentId, payload);
      toast({ title: editing ? 'Changes saved' : 'Submitted for review', body: editing ? form.title : `${cat.label} · a reviewer checks your proof next` });
      onSaved?.();
      onClose();
    } catch (err) {
      if (err.errors) setErrors(err.errors);
      else toast({ title: 'Could not save', body: err.message, tone: 'error' });
    } finally {
      setBusy(false);
    }
  }

  const aria = (k) => ({ 'aria-invalid': errors[k] ? 'true' : undefined, 'aria-describedby': errors[k] ? `ach-${k}-error` : `ach-${k}-hint` });

  return (
    <Modal
      title={editing ? 'Edit achievement' : 'Add an achievement'}
      subtitle="It starts as pending. EXP is added once a reviewer approves it."
      onClose={onClose}
      wide
    >
      <form onSubmit={submit} noValidate>
        <div className="modal-body">
          <div className="field">
            <span className="field-label" id="ach-cat-label">Category</span>
            <div className="segmented" role="group" aria-labelledby="ach-cat-label">
              {CATEGORIES.map((c) => (
                <button type="button" key={c.key} aria-pressed={form.category === c.key} onClick={() => set('category')(c.key)} data-autofocus={form.category === c.key ? true : undefined}>
                  <Icon name={CATEGORY_ICON[c.key]} />
                  <span>{c.label === 'Competitive programming' ? 'Competitive prog.' : c.label}</span>
                  <span className="seg-w mono">×{c.weight}</span>
                </button>
              ))}
            </div>
          </div>

          <div className="form-grid">
            <div className="full">
              <Field id="ach-title" label="Title" hint={`For example: ${TITLE_HINT[form.category]}`} error={errors.title}>
                <input id="ach-title" className="input" value={form.title} onChange={set('title')} maxLength={120} {...aria('title')} />
              </Field>
            </div>
            <Field id="ach-organization" label={ORG_LABEL[form.category][0]} hint={ORG_LABEL[form.category][1]} error={errors.organization}>
              <input id="ach-organization" className="input" value={form.organization} onChange={set('organization')} maxLength={80} {...aria('organization')} />
            </Field>
            <Field id="ach-achieved_on" label="Date" hint="When you earned it" error={errors.achieved_on}>
              <input id="ach-achieved_on" type="date" className="input" value={form.achieved_on} max={today()} onChange={set('achieved_on')} {...aria('achieved_on')} />
            </Field>
            {usesPlatform && (
              <Field id="ach-platform" label={form.category === 'competitive' ? 'Platform' : 'Platform (optional)'} hint={form.category === 'competitive' ? 'Codeforces counts ×2.5, CodeChef ×1.8, LeetCode ×1.2' : 'Where you solved them'} error={errors.platform}>
                <select id="ach-platform" className="select" value={form.platform} onChange={set('platform')} {...aria('platform')}>
                  <option value="">Choose…</option>
                  {PLATFORMS.map((p) => <option key={p}>{p}</option>)}
                </select>
              </Field>
            )}
            <Field id="ach-score" label="Points you claim" hint="0 to 1000. The reviewer checks it against your proof." error={errors.score}>
              <input id="ach-score" type="number" inputMode="numeric" min="0" max="1000" step="1" className="input" value={form.score} onChange={set('score')} {...aria('score')} />
            </Field>
            <div className={usesPlatform ? 'full' : ''}>
              <Field id="ach-proof_url" label="Proof link" hint="Certificate, contest standing, repo or offer letter (https://…)" error={errors.proof_url}>
                <input id="ach-proof_url" type="url" className="input" placeholder="https://" value={form.proof_url} onChange={set('proof_url')} {...aria('proof_url')} />
              </Field>
            </div>
            <div className="full">
              <Field id="ach-description" label="What you did (optional)" hint={`${form.description.length}/300`} error={errors.description}>
                <textarea id="ach-description" className="textarea" value={form.description} onChange={set('description')} maxLength={300} {...aria('description')} />
              </Field>
            </div>
          </div>
        </div>
        <div className="modal-foot">
          <button type="button" className="btn btn-ghost" onClick={onClose}>Cancel</button>
          <button type="submit" className="btn btn-primary" disabled={busy}>
            {busy ? 'Saving…' : editing ? 'Save changes' : 'Submit for review'}
          </button>
        </div>
      </form>
    </Modal>
  );
}
