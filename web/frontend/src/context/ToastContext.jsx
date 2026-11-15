import { createContext, useCallback, useContext, useState } from 'react';
import Icon from '../components/Icon.jsx';

const ToastContext = createContext(() => {});

export function ToastProvider({ children }) {
  const [toasts, setToasts] = useState([]);

  const dismiss = useCallback((id) => {
    setToasts((t) => t.map((x) => (x.id === id ? { ...x, leaving: true } : x)));
    setTimeout(() => setToasts((t) => t.filter((x) => x.id !== id)), 300);
  }, []);

  const push = useCallback(
    ({ title, body, tone = 'ok', duration = 4200 }) => {
      const id = Math.random().toString(36).slice(2);
      setToasts((t) => [...t.slice(-2), { id, title, body, tone }]);
      setTimeout(() => dismiss(id), duration);
    },
    [dismiss]
  );

  return (
    <ToastContext.Provider value={push}>
      {children}
      <div className="toasts" role="status" aria-live="polite">
        {toasts.map((t) => (
          <div key={t.id} className={`toast${t.leaving ? ' leaving' : ''}`}>
            <div className={`ok ${t.tone === 'info' ? 'info' : t.tone === 'error' ? 'err' : ''}`}>
              <Icon name={t.tone === 'error' ? 'x' : t.tone === 'info' ? 'info' : 'check'} strokeWidth={3} />
            </div>
            <div>
              <b>{t.title}</b>
              {t.body && <span>{t.body}</span>}
            </div>
            <button className="icon-btn" onClick={() => dismiss(t.id)} aria-label="Dismiss notification">
              <Icon name="x" />
            </button>
          </div>
        ))}
      </div>
    </ToastContext.Provider>
  );
}

export function useToast() {
  return useContext(ToastContext);
}
