import { useEffect, useRef, useState } from 'react';

export function usePrefersReducedMotion() {
  const [reduced, setReduced] = useState(() => window.matchMedia('(prefers-reduced-motion: reduce)').matches);
  useEffect(() => {
    const mq = window.matchMedia('(prefers-reduced-motion: reduce)');
    const on = () => setReduced(mq.matches);
    mq.addEventListener('change', on);
    return () => mq.removeEventListener('change', on);
  }, []);
  return reduced;
}

// Number ko purani value se nayi tak animate karo (pehli baar `from` se)
export function useCountUp(target, { duration = 900, from = 0 } = {}) {
  const reduced = usePrefersReducedMotion();
  const [value, setValue] = useState(reduced ? target : from);
  const prev = useRef(from);
  useEffect(() => {
    if (reduced) { setValue(target); prev.current = target; return; }
    const start = performance.now();
    const a = prev.current;
    let raf;
    const tick = (now) => {
      const p = Math.min(1, (now - start) / duration);
      const e = 1 - Math.pow(1 - p, 3);
      setValue(a + (target - a) * e);
      if (p < 1) raf = requestAnimationFrame(tick);
      else prev.current = target;
    };
    raf = requestAnimationFrame(tick);
    return () => cancelAnimationFrame(raf);
  }, [target, duration, reduced]);
  return value;
}

// Bars khaali se shuru hoke apni value tak badhte hain
export function useMounted(delay = 60) {
  const [on, setOn] = useState(false);
  useEffect(() => {
    const t = setTimeout(() => setOn(true), delay);
    return () => clearTimeout(t);
  }, [delay]);
  return on;
}

export function useDocumentTitle(title) {
  useEffect(() => {
    document.title = title ? `${title} · GEHU Forge` : 'GEHU Forge';
  }, [title]);
}
