import { createContext, useCallback, useContext, useEffect, useMemo, useState } from 'react';
import * as api from '../api/index.js';

const AuthContext = createContext(null);

export function AuthProvider({ children }) {
  const [session, setSession] = useState(() => api.getSession());
  const [me, setMe] = useState(null);
  const [dataVersion, setDataVersion] = useState(0);

  useEffect(() => {
    const bump = () => setDataVersion((v) => v + 1);
    const out = () => setSession(null);
    window.addEventListener('forge:data', bump);
    window.addEventListener('forge:logout', out);
    return () => {
      window.removeEventListener('forge:data', bump);
      window.removeEventListener('forge:logout', out);
    };
  }, []);

  useEffect(() => {
    let alive = true;
    if (!session) { setMe(null); return; }
    api.getMe()
      .then((m) => alive && setMe(m))
      .catch((e) => {
        // Network gaya toh session mat udao, sirf 401 pe logout
        if (e.code === 'unauthorized') { api.signOut(); if (alive) setSession(null); }
      });
    return () => { alive = false; };
  }, [session, dataVersion]);

  const signIn = useCallback(async (creds) => setSession(await api.signIn(creds)), []);
  const signInDemo = useCallback(async (role) => setSession(await api.signInDemo(role)), []);
  const signUp = useCallback(async (form) => setSession(await api.signUp(form)), []);
  const signOut = useCallback(() => { api.signOut(); setSession(null); }, []);

  const value = useMemo(
    () => ({ session, me, studentId: session?.studentId ?? null, role: session?.role ?? null, dataVersion, signIn, signInDemo, signUp, signOut }),
    [session, me, dataVersion, signIn, signInDemo, signUp, signOut]
  );
  return <AuthContext.Provider value={value}>{children}</AuthContext.Provider>;
}

export function useAuth() {
  return useContext(AuthContext);
}
