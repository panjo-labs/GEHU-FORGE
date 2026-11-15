// UI ke liye category/tier info. EXP aur rank C++ engine nikalta hai (backend ke through).

export const CATEGORIES = [
  { key: 'competitive', label: 'Competitive programming', short: 'CP', weight: 3.5 },
  { key: 'dsa', label: 'DSA', short: 'DSA', weight: 3.0 },
  { key: 'project', label: 'Projects', short: 'Projects', weight: 2.5 },
  { key: 'internship', label: 'Experience', short: 'Experience', weight: 5.0 },
  { key: 'certification', label: 'Learning', short: 'Learning', weight: 1.0 },
  { key: 'academic', label: 'Academic', short: 'Academic', weight: 1.5 },
];
export const CATEGORY = Object.fromEntries(CATEGORIES.map((c) => [c.key, c]));

export const PLATFORMS = ['Codeforces', 'CodeChef', 'LeetCode', 'HackerRank', 'Other'];
export const PLATFORM_MULTIPLIER = { codeforces: 2.5, codechef: 1.8, leetcode: 1.2, hackerrank: 0.8 };

export const TIERS = [
  { name: 'Candidate', min: 0, color: 'var(--tier-candidate)', hex: '#8A8F98' },
  { name: 'Specialist', min: 2000, color: 'var(--tier-specialist)', hex: '#2ED3B7' },
  { name: 'Expert', min: 4500, color: 'var(--tier-expert)', hex: '#4D8DFF' },
  { name: 'Master', min: 7000, color: 'var(--tier-master)', hex: '#FF7A45' },
  { name: 'Grandmaster', min: 9000, color: 'var(--tier-grandmaster)', hex: '#FFC93C' },
];

export const MAX_EXP = 10000;
