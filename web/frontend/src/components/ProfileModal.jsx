import { useRef, useState } from 'react';
import Icon from './Icon.jsx';
import { Modal, Field, Switch, Avatar } from './ui.jsx';
import * as api from '../api/index.js';
import { useAuth } from '../context/AuthContext.jsx';
import { useToast } from '../context/ToastContext.jsx';

function resizeImage(file) {
  return new Promise((resolve, reject) => {
    if (!file.type.startsWith('image/')) return reject(new Error('Choose an image file (JPG, PNG or WebP).'));
    const img = new Image();
    const url = URL.createObjectURL(file);
    img.onload = () => {
      const size = 256;
      const c = document.createElement('canvas');
      c.width = c.height = size;
      const ctx = c.getContext('2d');
      const s = Math.min(img.width, img.height);
      ctx.drawImage(img, (img.width - s) / 2, (img.height - s) / 2, s, s, 0, 0, size, size);
      URL.revokeObjectURL(url);
      resolve(c.toDataURL('image/jpeg', 0.85));
    };
    img.onerror = () => reject(new Error('This image could not be read.'));
    img.src = url;
  });
}

export default function ProfileModal({ student, onClose, onSaved }) {
  const { studentId } = useAuth();
  const toast = useToast();
  const fileRef = useRef(null);
  const [form, setForm] = useState(() => ({
    name: student.name || '', headline: student.headline || '', bio: student.bio || '', skills: student.skills || [],
    github: student.github || '', linkedin: student.linkedin || '', leetcode: student.leetcode || '',
    codeforces: student.codeforces || '', codechef: student.codechef || '',
    cf_rating: student.cf_rating ?? '', dsa_easy: student.dsa_easy ?? 0, dsa_medium: student.dsa_medium ?? 0, dsa_hard: student.dsa_hard ?? 0,
    show_profile: student.show_profile !== false, show_achievements: student.show_achievements !== false, show_rank: student.show_rank !== false,
    avatar_url: student.avatar_url || null,
  }));
  const [skill, setSkill] = useState('');
  const [errors, setErrors] = useState({});
  const [busy, setBusy] = useState(false);
  const set = (k) => (e) => setForm((f) => ({ ...f, [k]: e?.target ? e.target.value : e }));

  function addSkill(e) {
    e?.preventDefault();
    const v = skill.trim();
    if (!v) return;
    if (form.skills.length >= 10) return setErrors((x) => ({ ...x, skills: 'Up to 10 skills.' }));
    if (!form.skills.some((s) => s.toLowerCase() === v.toLowerCase())) setForm((f) => ({ ...f, skills: [...f.skills, v.slice(0, 24)] }));
    setSkill('');
  }

  async function onPhoto(e) {
    const file = e.target.files?.[0];
    if (!file) return;
    try {
      const url = await resizeImage(file);
      setForm((f) => ({ ...f, avatar_url: url }));
    } catch (err) {
      toast({ title: 'Photo not added', body: err.message, tone: 'error' });
    }
    e.target.value = '';
  }

  async function submit(e) {
    e.preventDefault();
    setBusy(true);
    setErrors({});
    try {
      const res = await api.updateProfile(studentId, form);
      toast({ title: 'Profile saved' });
      onSaved?.(res);
      onClose();
    } catch (err) {
      if (err.errors) setErrors(err.errors);
      else toast({ title: 'Could not save', body: err.message, tone: 'error' });
    } finally {
      setBusy(false);
    }
  }

  const inv = (k) => ({ 'aria-invalid': errors[k] ? 'true' : undefined });

  return (
    <Modal title="Edit profile" subtitle="This is what other students see on your profile and card." onClose={onClose} wide>
      <form onSubmit={submit} noValidate>
        <div className="modal-body">
          <div className="photo-row">
            <Avatar student={{ ...student, ...form }} size={72} />
            <div>
              <div style={{ display: 'flex', gap: 8, flexWrap: 'wrap' }}>
                <button type="button" className="btn btn-ghost btn-sm" onClick={() => fileRef.current?.click()}>
                  <Icon name="camera" /> {form.avatar_url ? 'Change photo' : 'Add photo'}
                </button>
                {form.avatar_url && <button type="button" className="btn btn-quiet btn-sm" onClick={() => set('avatar_url')(null)}>Remove</button>}
              </div>
              <span className="hint">Square photos work best. It is cropped to a circle.</span>
              <input ref={fileRef} type="file" accept="image/*" className="sr-only" onChange={onPhoto} aria-label="Upload profile photo" />
            </div>
          </div>

          <div className="form-grid">
            <Field id="pf-name" label="Full name" error={errors.name}>
              <input id="pf-name" className="input" value={form.name} onChange={set('name')} maxLength={80} {...inv('name')} data-autofocus />
            </Field>
            <Field id="pf-headline" label="Headline" hint="One line, e.g. Backend and competitive programming" error={errors.headline}>
              <input id="pf-headline" className="input" value={form.headline} onChange={set('headline')} maxLength={80} {...inv('headline')} />
            </Field>
            <div className="full">
              <Field id="pf-bio" label="Bio" hint={`${form.bio.length}/200`} error={errors.bio}>
                <textarea id="pf-bio" className="textarea" value={form.bio} onChange={set('bio')} maxLength={200} {...inv('bio')} />
              </Field>
            </div>
            <div className="full field">
              <label htmlFor="pf-skill">Skills</label>
              <div className="chips-input">
                {form.skills.map((s) => (
                  <span className="chip" key={s}>
                    {s}
                    <button type="button" aria-label={`Remove ${s}`} onClick={() => setForm((f) => ({ ...f, skills: f.skills.filter((x) => x !== s) }))}>×</button>
                  </span>
                ))}
                <input id="pf-skill" className="input" value={skill} placeholder={form.skills.length >= 10 ? 'Limit reached' : 'Type a skill and press Enter'} onChange={(e) => setSkill(e.target.value)} onKeyDown={(e) => e.key === 'Enter' && addSkill(e)} disabled={form.skills.length >= 10} />
                <button type="button" className="btn btn-ghost btn-sm" onClick={addSkill} disabled={!skill.trim()}>Add</button>
              </div>
              {errors.skills ? <span className="error">{errors.skills}</span> : <span className="hint">{form.skills.length}/10</span>}
            </div>
          </div>

          <h3 className="form-section">Links and handles</h3>
          <div className="form-grid">
            <Field id="pf-linkedin" label="LinkedIn" error={errors.linkedin} hint="https://www.linkedin.com/in/…">
              <input id="pf-linkedin" className="input" value={form.linkedin} onChange={set('linkedin')} {...inv('linkedin')} />
            </Field>
            <Field id="pf-github" label="GitHub username" error={errors.github}>
              <input id="pf-github" className="input" value={form.github} onChange={set('github')} {...inv('github')} />
            </Field>
            <Field id="pf-leetcode" label="LeetCode username" error={errors.leetcode}>
              <input id="pf-leetcode" className="input" value={form.leetcode} onChange={set('leetcode')} {...inv('leetcode')} />
            </Field>
            <Field id="pf-codeforces" label="Codeforces handle" error={errors.codeforces}>
              <input id="pf-codeforces" className="input" value={form.codeforces} onChange={set('codeforces')} {...inv('codeforces')} />
            </Field>
            <Field id="pf-codechef" label="CodeChef handle" error={errors.codechef}>
              <input id="pf-codechef" className="input" value={form.codechef} onChange={set('codechef')} {...inv('codechef')} />
            </Field>
            <Field id="pf-cf_rating" label="Codeforces rating" error={errors.cf_rating} hint="Leave empty if unrated">
              <input id="pf-cf_rating" type="number" min="0" max="4000" className="input" value={form.cf_rating} onChange={set('cf_rating')} {...inv('cf_rating')} />
            </Field>
          </div>

          <h3 className="form-section">DSA problems solved</h3>
          <div className="form-grid three">
            {[['dsa_easy', 'Easy'], ['dsa_medium', 'Medium'], ['dsa_hard', 'Hard']].map(([k, l]) => (
              <Field key={k} id={`pf-${k}`} label={l} error={errors[k]}>
                <input id={`pf-${k}`} type="number" min="0" className="input" value={form[k]} onChange={set(k)} {...inv(k)} />
              </Field>
            ))}
          </div>

          <h3 className="form-section">Privacy</h3>
          <div>
            <Switch id="pf-show_profile" checked={form.show_profile} onChange={set('show_profile')} label="Show my profile" detail="Other students can open your profile and card." />
            <Switch id="pf-show_rank" checked={form.show_rank} onChange={set('show_rank')} label="Show me on the leaderboard" detail="You still see your own rank." />
            <Switch id="pf-show_achievements" checked={form.show_achievements} onChange={set('show_achievements')} label="Show my achievements" detail="Only approved ones are ever shown to others." />
          </div>
        </div>
        <div className="modal-foot">
          <button type="button" className="btn btn-ghost" onClick={onClose}>Cancel</button>
          <button type="submit" className="btn btn-primary" disabled={busy}>{busy ? 'Saving…' : 'Save profile'}</button>
        </div>
      </form>
    </Modal>
  );
}
