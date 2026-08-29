# 🚀 GEU Forge: Student Talent & Ranking Platform

A high-performance C++ backend engine for student achievement tracking, skill profiling, dynamic multi-factor scoring, and real-time leaderboard ranking.

---

## 👥 Contributors & Roles

* **Pranjal Saraswat** — *Project Leader & Lead Systems Architect*  
  *(Architected the dynamic `RankEngine`, mathematical scoring system, logarithmic feature scaling, and hybrid Hash Map + Balanced BST ranking engine)*
* **Divyanshi Purohit** — *Core Developer: User & Student Module*  
  *(Designed `User` and `Student` domain models, encapsulation, skill registry via hash sets, and student profile views)*
* **Manya Singh** — *Core Developer: Achievement Hierarchy*  
  *(Built polymorphic `Achievement` base & derived classes, verification workflows, and category metadata)*

---

## 🏗️ System Architecture

The system is designed with strict **Object-Oriented Programming (OOP)** and efficient **Data Structures & Algorithms (DSA)**:

```
                  +-------------------+
                  |      User         | (Base Class)
                  +---------+---------+
                            |
                            v
                  +-------------------+       +-----------------------------+
                  |     Student       | ----> | std::unordered_set<string>  | (Skills: O(1) avg)
                  +---------+---------+       +-----------------------------+
                            |
                            | (Stores Base Pointers: vector<Achievement*>)
                            v
                  +-------------------+
                  |    Achievement    | (Polymorphic Base)
                  +----+----+----+----+
                       |    |    |    |
        +--------------+    |    |    +---------------+
        v                   v    v                    v
+---------------+  +------------+  +--------------+  +---------------+
| Coding        |  | Certificate|  | OpenSource   |  | Internship    |
| (CP: 3.5x)    |  | (1.0x)     |  | (2.5x)       |  | (5.0x)        |
+---------------+  +------------+  +--------------+  +---------------+
                            |
                            | (Processed & Verified)
                            v
                 +----------------------+
                 |      RankEngine      |
                 +----------+-----------+
                            |
            +---------------+---------------+
            v                               v
+------------------------+      +-------------------------------------------+
| std::unordered_map     |      | std::set<pair<double, int>, greater<...>> |
| (O(1) StudentId -> EXP)|      | (O(log N) Red-Black Tree Leaderboard)     |
+------------------------+      +-------------------------------------------+
```

---

## 📂 File Structure & Role of Each File

| File | Purpose & Role |
| :--- | :--- |
| **[`Achievement.h`](file:///c:/Users/pranjal/Desktop/Geu%20Forge/Achievement.h)** | Defines the `Achievement` base class and derived types (`CodingAchievement`, `CertificateAchievement`, `OpenSourceAchievement`, `InternshipAchievement`). Encapsulates verification status, scores, and category tags. |
| **[`User.h`](file:///c:/Users/pranjal/Desktop/Geu%20Forge/User.h)** | Defines the `User` base class and `Student` entity. Manages duplicate-free skills (`std::unordered_set`), handles (`GitHub`, `LeetCode`), and links to achievements (`std::vector<Achievement*>`). |
| **[`RankEngine.h`](file:///c:/Users/pranjal/Desktop/Geu%20Forge/RankEngine.h)** | Core scoring & leaderboard engine. Handles category multipliers, platform weights, anti-spam decay, logarithmic normalization ($\text{EXP} \in [0, 10000]$), $\mathcal{O}(1)$ hash lookups, and $\mathcal{O}(\log N)$ dynamic BST re-ranking. |
| **[`main.cpp`](file:///c:/Users/pranjal/Desktop/Geu%20Forge/main.cpp)** | Simulation and testing driver. Demonstrates student registration, credential verification, anti-spam decay handling, $\mathcal{O}(K)$ top talent querying, and dynamic leaderboard updates. |

---

## ⚡ Mathematical Scoring & Dynamic Weighting

1. **Category Multipliers:**
   * **Internships / Industry Experience:** $5.0\times$
   * **Competitive Programming (CP):** $3.5\times$
   * **Open Source / Dev Contributions:** $2.5\times$
   * **Coursework & Certifications:** $1.0\times$

2. **Platform Benchmarking Multipliers:**
   * **Codeforces:** $2.5\times$ | **CodeChef:** $1.8\times$ | **LeetCode:** $1.2\times$ | **HackerRank:** $0.8\times$

3. **Anti-Spam Daily Activity Decay Curve:**
   * Submissions $1 - 5$: $1.0\times$ (100% full credit)
   * Submissions $6 - 10$: $0.4\times$ (40% credit / diminishing returns)
   * Submissions $11+$: $0.1\times$ (10% spam protection cap)

4. **Logarithmic Feature Scaling (Score Normalization):**
   $$\text{EXP} = \min\left(10000.0, 10000.0 \times \frac{\ln(1 + \text{rawScore})}{\ln(1 + 50000.0)}\right)$$

---

## ⏱️ Computational Complexities

| Operation | Method / API | Time Complexity | Space Complexity |
| :--- | :--- | :--- | :--- |
| **Skill Add / Remove** | `Student::addSkill` / `removeSkill` | $\mathcal{O}(1)$ avg | $\mathcal{O}(S)$ |
| **Process & Re-rank** | `RankEngine::processAchievement` | $\mathcal{O}(\log N)$ | $\mathcal{O}(1)$ aux |
| **Top-K Retrieval** | `RankEngine::getTopK` | $\mathcal{O}(K)$ | $\mathcal{O}(K)$ |
| **Rank Query** | `RankEngine::getStudentRank` | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ aux |
| **Display Table** | `RankEngine::displayLeaderboard` | $\mathcal{O}(N)$ | $\mathcal{O}(1)$ aux |

---

## 🛠️ How to Build & Run

### Prerequisites
* C++17 compatible compiler (`g++`, `clang++`, or `MSVC`)

### Compile and Execute
```bash
# Compile using g++
g++ -std=c++17 main.cpp -o main.exe

# Run the executable (Windows)
.\main.exe

# Run the executable (Linux/macOS)
./main.exe
```
