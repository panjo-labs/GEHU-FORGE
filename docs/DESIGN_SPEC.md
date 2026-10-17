# GEHU Forge 2 — Design Spec (no STL containers)

Rule: the program uses NO STL containers (no vector, map, set, queue, unordered_*, list, stack, deque,
pair, unique_ptr). `std::string`, `<iostream>`, `<fstream>`, `<cmath>`, `<iomanip>`, `<cstdlib>` are allowed.
Tests (`tests/`) may use STL ONLY as an independent "oracle" to cross-check our own structures.

Build: `g++ -std=c++17 -Wall -Wextra main.cpp -o forge.exe`   Run: `forge.exe` or `forge.exe --demo`

## Files and owners

| File | Owner | Contents |
|---|---|---|
| `ds/DynArray.h` | Saksham | `template<class T> class DynArray` |
| `ds/HashMap.h` | Saksham | `hashKey()` overloads + `template<class K,class V> class HashMap` |
| `ds/HashSet.h` | Saksham | `template<class T> class HashSet` (built on HashMap by composition) |
| `Storage.h` | Saksham | `class DataStore` (owns all objects + CSV load/save), `class DataGenerator` (seeded synthetic data) |
| `ds/LinkedQueue.h` | Manya | `template<class T> class LinkedQueue` |
| `Achievement.h` | Manya | enums `AchievementCategory`, `VerificationStatus`; abstract `Achievement`; `CodingAchievement`, `CertificateAchievement`, `OpenSourceAchievement`, `InternshipAchievement` |
| `Verification.h` | Manya | `class VerificationService` (queue + review workflow) |
| `ds/AVLTree.h` | Pranjal | `struct RankKey`, `class AVLTree` (self-balancing BST with subtree sizes) |
| `RankEngine.h` | Pranjal | `class RankEngine` (scoring maths, decay, leaderboard, rank queries) |
| `User.h` | Divyanshi | abstract `User`, `Student` (+ profile card) |
| `Cli.h` | Divyanshi | `class App` (menu, input validation, demo mode) |
| `main.cpp` | Divyanshi | tiny entry point |
| `tests/` | Saksham (structures) + Pranjal (pipeline) | `test_structures.cpp`, `test_pipeline.cpp`, `TestUtil.h` |

## System flow (who touches what)

```
 data/students.csv + data/achievements.csv   (100 synthetic students, seed 2026)
        |  Saksham: DataStore::load()  (CSV parsing, DataGenerator if files missing)
        v
 DataStore ── owns ──> HashMap<int,Student*>  and  HashMap<int,Achievement*>     (Saksham)
        |
        |  CLI "Submit achievement"  (Divyanshi: App validates input, builds object)
        v
 Achievement object, status = Pending                                              (Manya)
        |  VerificationService::submit(a)  -> LinkedQueue<int> (FIFO of achievement ids)
        v
 CLI "Review next"  -> VerificationService::reviewNext(registry, approve)          (Manya)
        |  approved?  yes -> Achievement::approve() ; no -> reject()  (state machine)
        v
 RankEngine::applyAchievement(student, ach)                                        (Pranjal)
        |  ach.tryConsumeForScoring()  (score-once guard)
        |  weighted points = base * categoryWeight * platformMultiplier * dailyDecay
        |  raw score -> log normalisation -> EXP (0..10000)
        |  HashMap erase/update O(1)  +  AVLTree erase+insert O(log N)
        v
 AVLTree leaderboard  ->  getTopK(10) O(K + log N),  getRank(id) O(log N)
        v
 CLI prints leaderboard / profile card (Student::printProfileCard)                 (Divyanshi)
```

## Exact APIs (guides and code MUST use these names)

### DynArray<T>  (Saksham) — amortised O(1) append, O(1) index
- `DynArray()`, `DynArray(const DynArray&)` (deep copy), `operator=(const DynArray&)`, `~DynArray()`  (Rule of Three)
- `void push_back(const T&)` amortised O(1) (capacity doubles when full, starting at 4)
- `T& operator[](int i)`, `const T& operator[](int i) const` O(1)
- `int size() const`, `int capacity() const`, `bool empty() const`, `void clear()`
- `bool removeAt(int i)` O(n) (shifts left)

### HashMap<K,V>  (Saksham) — separate chaining
- `unsigned long hashKey(int)`, `unsigned long hashKey(const std::string&)` (polynomial rolling hash, base 31)
- Node = `{K key; V value; Node* next;}`; table = array of `Node*` buckets; initial capacity 16;
  rehash (double the bucket count, re-insert every node) when `size/capacity > 0.75`
- `void put(const K&, const V&)` insert or overwrite, avg O(1)
- `V* find(const K&)`, `const V* find(const K&) const`  -> pointer to value or `nullptr`, avg O(1)
- `bool contains(const K&) const`, `bool remove(const K&)`, `int size() const`, `int bucketCount() const`
- `double loadFactor() const`
- `template<class F> void forEach(F f) const`  (calls `f(const K&, const V&)`)
- Rule of Three: destructor frees every chain; copy constructor/assignment are deleted (`= delete`)

### HashSet<T>  (Saksham) — duplicate-free set, wraps `HashMap<T,bool>`
- `bool add(const T&)` (false if already present), `bool contains(const T&) const`, `bool remove(const T&)`,
  `int size() const`, `template<class F> void forEach(F f) const`

### LinkedQueue<T>  (Manya) — FIFO with front + rear pointers
- Node = `{T data; Node* next;}`; members `Node* frontNode, *rearNode; int count;`
- `void enqueue(const T&)` O(1), `bool dequeue(T& out)` O(1) (false when empty — never dereferences null),
  `bool peek(T& out) const` O(1), `bool isEmpty() const`, `int size() const`
- Destructor frees remaining nodes; copy constructor and copy assignment are `= delete` (Rule of Three/Five)

### Achievement hierarchy  (Manya)
- `enum class AchievementCategory { INTERNSHIP, COMPETITIVE_PROGRAMMING, OPEN_SOURCE, CERTIFICATION, GENERAL }`
- `enum class VerificationStatus { Pending, Approved, Rejected }`
- `class Achievement` (ABSTRACT). Private-ish state via protected members: `id, studentId, title, date ("DD-MM-YYYY"),
  score (int), proof, status (starts Pending), pointsApplied (starts false), category`
  - pure virtual: `void display() const = 0`, `double categoryWeight() const = 0`, `std::string typeName() const = 0`
  - virtual with default: `double platformMultiplier() const` (returns 1.0), `double basePoints() const` (returns score)
  - non-virtual: `double weightedPoints() const` = if basePoints<0: basePoints*0.3 (cushioned penalty) else
    basePoints*categoryWeight*platformMultiplier
  - state machine (only Pending can change): `bool approve()`, `bool reject()` return false if not Pending
  - `bool tryConsumeForScoring()` : true only if Approved AND !pointsApplied; sets pointsApplied=true atomically (check-and-set)
  - getters: `getId, getStudentId, getTitle, getDate, getScore, getProof, getStatus, isApproved, isPointsApplied, getCategory`
  - `static const char* statusName(VerificationStatus)`
  - `virtual ~Achievement()`
- `CodingAchievement(id, studentId, title, date, score, proof, platform)` : weight 3.5; platformMultiplier from
  platform string (codeforces 2.5, codechef 1.8, leetcode 1.2, hackerrank 0.8, else 1.0); `getPlatform()`; typeName "CODING"
- `CertificateAchievement` weight 1.0, typeName "CERT"; `OpenSourceAchievement` weight 2.5, "OSS"; `InternshipAchievement` weight 5.0, "INTERN"
- `Achievement* createAchievement(const std::string& type, id, studentId, title, date, score, proof, platform)` — factory
  (returns nullptr for unknown type) used by CSV loader and CLI

### VerificationService  (Manya)
- member `LinkedQueue<int> pending` (achievement ids, FIFO)
- `bool submit(Achievement* a)` : requires non-null and status Pending (else returns false); enqueues the id
- `Achievement* reviewNext(const HashMap<int,Achievement*>& registry, bool approve)` : dequeue oldest id, look it up
  (O(1)), approve or reject, return it; nullptr if queue empty or id unknown
- `bool peekNextId(int& id) const`, `int pendingCount() const`, `bool hasPending() const`

### AVLTree  (Pranjal)
- `struct RankKey { double exp; int studentId; }`. Ordering `before(a,b)`: `a.exp > b.exp || (a.exp == b.exp && a.studentId < b.studentId)`
  — in-order traversal therefore yields rank 1 first.
- Node = `{RankKey key; int height; int size; Node* left,*right;}` (size = nodes in subtree)
- `void insert(const RankKey&)` O(log N), `bool erase(const RankKey&)` O(log N)
- `int rankOf(const RankKey&) const` O(log N): 1-based rank using subtree sizes; -1 if absent
- `void topK(int k, DynArray<RankKey>& out) const` : first k keys in order, O(K + log N)
- `int size() const`, `int height() const`, `bool isBalanced() const` (verifies AVL property, for tests)
- rotations: `rotateLeft`, `rotateRight`; cases LL, RR, LR, RL; Rule of Three: destructor frees all nodes, copy deleted

### RankEngine  (Pranjal)
- members: `HashMap<int,double> rawScore`, `HashMap<int,double> expMap`, `HashMap<std::string,int> dailyCount` (key "studentId|date"), `AVLTree board`
- `bool applyAchievement(Student& s, Achievement& a)` : false if not approved or already applied; else
  count = ++dailyCount[studentId|date]; decay = 1.0 (count<=5), 0.4 (6..10), 0.1 (11+);
  delta = a.weightedPoints() * (delta>0 ? decay : 1); raw = max(0, raw+delta);
  EXP = min(10000, 10000 * ln(1+raw)/ln(1+50000)); old key erased from AVLTree via expMap lookup, new key inserted
- `DynArray<RankKey> getTopK(int k) const`, `int getRank(int studentId) const` (-1 if unranked), `double getExp(int) const`,
  `int rankedCount() const`, `static std::string tierBadge(double exp)`
- tiers: >=9000 Grandmaster, >=7000 Master, >=4500 Expert, >=2000 Specialist, else Candidate

### User / Student  (Divyanshi)
- `class User` ABSTRACT: protected `name, email`; ctor(name,email); `virtual ~User()`; `virtual std::string getRole() const = 0`; getters
- `class Student : public User`: private `studentId, branch, graduationYear, githubHandle, leetcodeHandle,
  HashSet<std::string> skills, DynArray<Achievement*> achievements (non-owning pointers)`
- `getRole()` returns "Student"; `addSkill/removeSkill/hasSkill`, `getSkills() const` (const ref), setters/getters,
  `void linkAchievement(Achievement*)` (ignores nullptr and duplicates by id), `getAchievements() const`,
  `void printProfileCard(int rank, double exp, const std::string& tier) const`

### DataStore / DataGenerator  (Saksham)
- `DataStore`: `HashMap<int,Student*> students`, `HashMap<int,Achievement*> achievements`, `DynArray<int> studentOrder`,
  `DynArray<int> achievementOrder` (insertion order, for stable CSV output), `int nextStudentId, nextAchievementId`
  - owns all heap objects: destructor deletes every Student and Achievement
  - `bool addStudent(Student* s)` (returns false and does NOT take ownership if the id already exists), `bool addAchievement(Achievement* a)`
  - `Student* findStudent(int)`, `Achievement* findAchievement(int)` (+ const overloads), read-only accessors `achievementRegistry()`, `studentIds()`, `achievementIds()`, `int studentCount() const`, `int achievementCount() const`
  - `bool loadFromCsv(const std::string& studentsPath, const std::string& achievementsPath)`, `bool saveToCsv(...) const`
  - `int newStudentId()`, `int newAchievementId()`; `void clear()`
- CSV `students.csv` header `id,name,email,branch,gradYear,github,leetcode,skills` (skills joined with `;`)
- CSV `achievements.csv` header `id,studentId,type,title,date,score,platform,proof,status` (commas removed from free text)
- `static DataGenerator::generate(DataStore&, int studentCount = 100, unsigned seed = 2026)` : linear-congruential generator
  (own code, not rand()), fake names/branches/skills, 1–3 achievements per student; ~80% Approved, ~12% Pending, ~8% Rejected
- helper `splitLine(const std::string&, char delim, DynArray<std::string>& out)`

### App / CLI  (Divyanshi)
- Menu: 1 Top 10, 2 Top K, 3 Student profile, 4 Add student, 5 Submit achievement, 6 Review next pending,
  7 Rank of a student, 8 Dataset statistics, 9 Save, 10 Reset to synthetic seed, 0 Save and exit; `--demo` scripted run
- On start: load CSVs from `data/` (generate if missing); every Approved & not-yet-applied achievement is scored through
  RankEngine in file order; Pending ones are enqueued in VerificationService
- Input helpers validate: integer range, non-empty text, no commas, unknown student id, unknown type, duplicate

## Implementation notes (added after coding)
- `RankEngine` also exposes `boardHeight()` and `boardIsBalanced()` (used by the stats menu and the tests).
- Achievement ids in CSV are saved with their status (`Pending`, `Approved`, `Rejected`); at start-up Approved rows are scored in file order and Pending rows are re-queued in file order.
- Tests: `tests/test_structures.cpp` (46 checks) and `tests/test_pipeline.cpp` (57 checks); STL is used there only as an oracle.
