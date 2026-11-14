# GEHU Forge: Student Talent and Ranking Platform

A competitive profile platform for Graphic Era Hill University. Students add contests, DSA progress, projects, internships, certificates and academic wins. A reviewer verifies each one, and a C++ engine turns approved work into EXP (0 to 10,000), category ranks and a live college leaderboard.

```
React website (web/frontend)
        |  /api
FastAPI backend (web/backend)  ---->  SQLite now, Supabase Postgres later (DATABASE_URL)
        |  stdin / stdout
C++ engine (engine/forge_engine.cpp)
   RankEngine: AVL tree leaderboards, overall + 6 categories
   SearchEngine: Trie + inverted index + edit distance + min-heap top-K
```

The C++ core uses **no STL containers**. Every data structure is our own (`ds/`). `std::string`, streams and `<cmath>` are used. Tests use STL only as an independent checker.

## Team and ownership

| Member | Role | Owns |
|---|---|---|
| **Pranjal Saraswat** | **Team lead, architecture, ranking and search engines, full-stack integration** | `ds/AVLTree.h`, `ds/Trie.h`, `ds/MinHeap.h`, `RankEngine.h`, `SearchEngine.h`, `engine/forge_engine.cpp`, `web/backend` (API, auth, engine bridge, reviewer API), `web/frontend` (all pages), database design, `tests/test_pipeline.cpp`, `tests/test_search.cpp`, `web/backend/tests` |
| Saksham Singh | Core containers and data | `ds/DynArray.h`, `ds/HashMap.h`, `ds/HashSet.h`, `Storage.h` (CSV load/save, synthetic data generator), `tests/test_structures.cpp` |
| Manya Singh | Achievement model and verification | `Achievement.h`, `ds/LinkedQueue.h`, `Verification.h`, first FastAPI verification module (used as the reference for the final reviewer API) |
| Divyanshi Purohit | Student model and console app | `User.h` (User, Student, profile card), `Cli.h`, `main.cpp` |

### Pranjal Saraswat (lead)

- **Architecture:** designed how the C++ engine, the Python API, the database and the website fit together, and the API contract all modules follow.
- **AVL tree leaderboard (`ds/AVLTree.h`):** self-balancing BST with rotations and subtree sizes. Insert, erase and rank are O(log N); Top-K is O(K + log N).
- **RankEngine (`RankEngine.h`):** weighted points, daily anti-spam decay, log normalisation to EXP, and the score-once guard. Uses a HashMap for the old EXP (O(1)) and the AVL tree to re-sort (O(log N)). Feeds one overall board and six category boards.
- **Search engine (`SearchEngine.h`, `ds/Trie.h`, `ds/MinHeap.h`):**
  - A Trie for prefix search on names, roll numbers, skills and headlines, with postings at each word (an inverted index).
  - Edit distance (dynamic programming) so small typos still match.
  - A binary min-heap that keeps the best K results in O(N log K).
- **Engine server (`engine/forge_engine.cpp`):** a long-running process that the API talks to over stdin and stdout (`LOAD`, `RANK`, `SEARCH`). It also computes the table as of seven days ago, for rank movement.
- **Backend (`web/backend`):**
  - FastAPI with bcrypt passwords and JWT login, and student/reviewer roles.
  - Validation that mirrors the database rules.
  - The reviewer queue: oldest first, a conditional update so only one reviewer wins, and automatic proof checks through free Codeforces and GitHub APIs.
  - The engine bridge, and API tests.
- **Website (`web/frontend`):** the "Arena" design, landing page, dashboard, leaderboard and search, profile and achievement forms, shareable card, and the reviewer page.
- **OOP shown:** encapsulation (RankEngine and SearchEngine keep their maps and trees private), composition (RankEngine has-a AVLTree and HashMaps; SearchEngine has-a Trie, HashSet and MinHeap), templates (`MinHeap<T, Less>`), Rule of Three (Trie and AVL tree own nodes, copying is disabled), and separation of concerns across engine, API and UI.

### Saksham Singh

- **DSA:** `DynArray<T>` (doubling growth, amortised O(1) push), `HashMap<K,V>` (separate chaining, rehash at load factor 0.75, average O(1)), `HashSet<T>` (built on the HashMap).
- **OOP:** class templates, Rule of Three with deep copy and safe self-assignment, composition (HashSet has-a HashMap), RAII destructors.
- **Data:** `DataStore` owns every object and saves to CSV; the seeded generator gives the same data every run. The web seed (`web/database/generate_seed.py`) extends this generator.
- **Tests:** 46 structure checks against STL as the reference.

### Manya Singh

- **OOP:** abstract base class `Achievement` with pure virtual functions. Six derived types (Coding, Certificate, Open source, Internship, DSA, Academic) override the weight and platform multiplier, which is runtime polymorphism. It has a virtual destructor and a template-method `weightedPoints()`.
- **DSA:** `LinkedQueue<T>`, a FIFO queue with front and rear pointers (O(1) enqueue and dequeue), which drives the review order.
- **Verification:** a one-way state machine (Pending to Approved or Rejected), the score-once guard, and the first FastAPI version of the submission and review endpoints.

### Divyanshi Purohit

- **OOP:** abstract `User` and derived `Student` (inheritance), a skills set and achievement list inside each student (composition), const-correct getters, and non-owning pointers.
- **DSA:** uses HashSet for duplicate-free skills and DynArray for achievement lists.
- **App:** the menu-driven console program with input validation, the profile card, and the `--demo` mode. The profile card design carried over to the website's shareable card.

## Run everything

```bash
# C++ engine and tests
g++ -std=c++17 -O2 engine/forge_engine.cpp -o engine/forge_engine.exe
g++ -std=c++17 -Wall -Wextra tests/test_structures.cpp -o test_structures.exe && ./test_structures.exe
g++ -std=c++17 -Wall -Wextra tests/test_pipeline.cpp  -o test_pipeline.exe  && ./test_pipeline.exe
g++ -std=c++17 -Wall -Wextra tests/test_search.cpp    -o test_search.exe    && ./test_search.exe

# API (web/backend), see web/backend/README.md
python -m venv .venv && .venv\Scripts\activate && pip install -r requirements.txt
uvicorn app.main:app --reload --port 8000

# Website (web/frontend)
npm install && npm run dev        # http://localhost:5173
```

The console version still works: `g++ -std=c++17 main.cpp -o forge.exe && ./forge.exe` (or `--demo`).

## Data structures

| Structure | File | Used for | Cost |
|---|---|---|---|
| `DynArray<T>` | `ds/DynArray.h` | growable lists | push amortised O(1), index O(1) |
| `HashMap<K,V>` | `ds/HashMap.h` | id lookups, EXP map, daily counters, search scores | average O(1) |
| `HashSet<T>` | `ds/HashSet.h` | skills, search vocabulary | average O(1) |
| `LinkedQueue<T>` | `ds/LinkedQueue.h` | review queue (FIFO) | O(1) |
| `AVLTree` | `ds/AVLTree.h` | leaderboards | insert/erase/rank O(log N), Top-K O(K + log N) |
| `Trie` | `ds/Trie.h` | prefix search with postings | insert/walk O(L) |
| `MinHeap<T, Less>` | `ds/MinHeap.h` | top-K search results | push/pop O(log K) |
| Edit distance | `SearchEngine.h` | typo matching | O(m·n) time, O(n) memory |

## Scoring

1. Category weight: Internship 5.0, Competitive programming 3.5, DSA 3.0, Projects 2.5, Academic 1.5, Certification 1.0.
2. Platform multiplier (competitive programming only): Codeforces 2.5, CodeChef 1.8, LeetCode 1.2, HackerRank 0.8, other 1.0.
3. Daily decay per student per date: items 1 to 5 at 100%, 6 to 10 at 40%, 11 and up at 10%.
4. `EXP = min(10000, 10000 · ln(1 + raw) / ln(1 + 50000))`.
5. Tiers: Candidate 0, Specialist 2,000, Expert 4,500, Master 7,000, Grandmaster 9,000.
6. Ties go to the smaller student id. Only approved achievements count, each exactly once.

## Known limitations

- Demo buttons (`DEMO_MODE`) are for evaluation and must be off in real use. There is no password reset or email verification yet.
- LeetCode, certificates and internships have no free verification API, so a reviewer checks those links by hand.
- The seeded students and achievements are synthetic. Their proof links are placeholders.
- With the current EXP curve most active students land in Master; a flatter curve is under discussion.
