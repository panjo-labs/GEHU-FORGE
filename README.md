# GEHU Forge 2: Student Talent and Ranking Platform (no STL containers)

A C++17 console application that tracks student achievements, verifies them through a FIFO
review queue, converts approved achievements into EXP (0 to 10,000) and keeps a live leaderboard.

**Phase 2 change:** every container is now written by us. The program uses **no STL containers**
(no `vector`, `map`, `set`, `queue`, `unordered_*`). `std::string`, file streams and `<cmath>` are still used.
The tests in `tests/` use STL only as an independent "oracle" to check our own structures.

## Team and ownership

| Member | Owns |
|---|---|
| Pranjal Saraswat (lead) | `ds/AVLTree.h`, `RankEngine.h`, pipeline tests, integration |
| Divyanshi Purohit | `User.h` (User, Student, profile card), `Cli.h`, `main.cpp` |
| Manya Singh | `Achievement.h`, `ds/LinkedQueue.h`, `Verification.h` |
| Saksham Singh | `ds/DynArray.h`, `ds/HashMap.h`, `ds/HashSet.h`, `Storage.h`, structure tests |

Study guides for each member (concepts, traces, viva questions) are in `study_material/`.

## Build and run

```bash
g++ -std=c++17 -Wall -Wextra main.cpp -o forge.exe      # build
./forge.exe                                              # interactive menu
./forge.exe --demo                                       # scripted demonstration

g++ -std=c++17 -Wall -Wextra tests/test_structures.cpp -o test_structures.exe && ./test_structures.exe
g++ -std=c++17 -Wall -Wextra tests/test_pipeline.cpp  -o test_pipeline.exe  && ./test_pipeline.exe
```

On first run the program generates 100 synthetic students (fixed seed 2026, fake names) into
`data/students.csv` and `data/achievements.csv`. Anything you add through the menu is saved there too.
Menu option 10 resets to the synthetic data.

## Data flow

```
data/*.csv --load--> DataStore (owns all objects, HashMap lookups)
CLI "submit" --> Achievement (Pending) --> VerificationService (LinkedQueue, FIFO)
CLI "review" --> approve / reject (state machine) --> RankEngine.applyAchievement
RankEngine: score-once guard -> weighted points -> daily decay -> log normalisation -> EXP
            HashMap (old EXP, O(1)) + AVLTree (erase + insert, O(log N)) --> leaderboard
CLI "top 10" / "rank" / "profile" read the AVLTree
```

## Data structures (all custom)

| Structure | File | Used for | Main costs |
|---|---|---|---|
| `DynArray<T>` | `ds/DynArray.h` | growable arrays | push_back amortised O(1), index O(1) |
| `HashMap<K,V>` | `ds/HashMap.h` | id lookups, EXP map, daily counters | put/find/remove average O(1) (chaining, rehash at load 0.75) |
| `HashSet<T>` | `ds/HashSet.h` | duplicate-free skills | add/contains/remove average O(1) |
| `LinkedQueue<T>` | `ds/LinkedQueue.h` | review queue (FIFO) | enqueue, dequeue, peek O(1) |
| `AVLTree` | `ds/AVLTree.h` | leaderboard | insert/erase/rank O(log N), top-K O(K + log N) |

## OOP concepts

- **Abstraction / abstract classes:** `Achievement` and `User` have pure virtual functions.
- **Inheritance:** four achievement types derive from `Achievement`; `Student` derives from `User`.
- **Polymorphism:** `display()`, `categoryWeight()`, `platformMultiplier()` are resolved at run time through `Achievement*`.
- **Encapsulation:** state changes only through checked methods (`approve`, `reject`, `tryConsumeForScoring`).
- **Rule of Three/Five, RAII:** classes that own memory free it in destructors; copying is deleted where unsafe.
- **Templates:** all containers are class templates.
- **Composition, separation of concerns:** `RankEngine` has-a `AVLTree` and `HashMap`s; verification, scoring, storage and UI are separate modules.

## Scoring

1. Category weight: Internship 5.0, Competitive programming 3.5, Open source 2.5, Certification 1.0.
2. Platform multiplier (competitive programming only): Codeforces 2.5, CodeChef 1.8, LeetCode 1.2, HackerRank 0.8, other 1.0.
3. Negative scores (rating loss) are cushioned to 30%.
4. Daily decay per student per date: items 1 to 5 at 100%, 6 to 10 at 40%, 11 and up at 10%.
5. `EXP = min(10000, 10000 * ln(1 + raw) / ln(1 + 50000))`.
6. Ties are broken by smaller student id.

Only **Approved** achievements score, and each achievement scores **at most once**.

## Known limitations (honest list)

- Daily decay depends on the date text (`DD-MM-YYYY`) the user enters; dates are not checked against a real calendar.
- Free text cannot contain `,` or `;` (they would break the CSV format).
- The review queue is rebuilt from the Pending rows in the CSV file when the program starts.
- Verification is a manual menu action; there is no login or admin role yet.
- The synthetic data is fake and only for demonstration.
- Not thread-safe, single user, console only.
