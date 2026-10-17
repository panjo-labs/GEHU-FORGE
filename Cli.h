#pragma once

#include <iostream>
#include <iomanip>
#include <string>
#include <filesystem>
#include "User.h"
#include "Achievement.h"
#include "Verification.h"
#include "RankEngine.h"
#include "Storage.h"

// ============================================================================
// App  -  the command-line interface (menu + input checking)
// ============================================================================
// SEPARATION OF CONCERNS: App only talks to the user and checks what they type.
// The real work is done by the other modules:
//    DataStore (objects + files)   VerificationService (queue)   RankEngine (EXP + ranks)
//
// App OWNS one DataStore, and creates one VerificationService and one RankEngine
// with 'new' so they can be thrown away and rebuilt (for "Reset").
class App {
private:
    DataStore store;
    RankEngine* engine;
    VerificationService* verifier;
    std::string studentsPath;
    std::string achievementsPath;
    bool unsavedChanges;

    // ------------------------------------------------------------------
    // INPUT HELPERS  (every user typing goes through these)
    // ------------------------------------------------------------------
    // Read a whole line. Returns false when input has ended (Ctrl+Z / end of file).
    // We read WHOLE LINES and convert ourselves, because 'cin >> number' breaks
    // badly when the user types letters.
    static bool readLine(const std::string& prompt, std::string& out) {
        std::cout << prompt;
        if (!std::getline(std::cin, out)) return false;
        size_t b = out.find_first_not_of(" \t\r");
        size_t e = out.find_last_not_of(" \t\r");
        out = (b == std::string::npos) ? "" : out.substr(b, e - b + 1);   // trim spaces
        return true;
    }

    // Is the text a whole number like "42" or "-7"?
    static bool parseInt(const std::string& text, int& value) {
        if (text.empty() || text.size() > 9) return false;                // 9 digits fit in int
        size_t i = (text[0] == '-') ? 1 : 0;
        if (i == text.size()) return false;
        for (size_t k = i; k < text.size(); ++k) {
            if (text[k] < '0' || text[k] > '9') return false;
        }
        value = std::atoi(text.c_str());
        return true;
    }

    // Keep asking until the user types a number in [lo, hi]. false = input ended.
    static bool readInt(const std::string& prompt, int lo, int hi, int& value) {
        std::string line;
        while (true) {
            if (!readLine(prompt, line)) return false;
            if (parseInt(line, value) && value >= lo && value <= hi) return true;
            std::cout << "  ! Please type a whole number from " << lo << " to " << hi << ".\n";
        }
    }

    static bool hasBadChar(const std::string& s) {
        return s.find(',') != std::string::npos || s.find(';') != std::string::npos;
    }

    // Keep asking until the text is non-empty and free of ',' and ';'.
    static bool readText(const std::string& prompt, std::string& out, bool allowEmpty = false) {
        while (true) {
            if (!readLine(prompt, out)) return false;
            if (out.empty() && !allowEmpty) { std::cout << "  ! This cannot be empty.\n"; continue; }
            if (hasBadChar(out)) { std::cout << "  ! Please do not use ',' or ';' (they break the CSV file).\n"; continue; }
            return true;
        }
    }

    // "DD-MM-YYYY" check (format and simple ranges).
    static bool validDate(const std::string& d) {
        if (d.size() != 10 || d[2] != '-' || d[5] != '-') return false;
        int day = 0, month = 0, year = 0;
        if (!parseInt(d.substr(0, 2), day) || !parseInt(d.substr(3, 2), month) ||
            !parseInt(d.substr(6, 4), year)) return false;
        return day >= 1 && day <= 31 && month >= 1 && month <= 12 && year >= 2000 && year <= 2100;
    }

    static bool readDate(const std::string& prompt, std::string& out) {
        while (true) {
            if (!readLine(prompt, out)) return false;
            if (validDate(out)) return true;
            std::cout << "  ! Use the format DD-MM-YYYY, e.g. 15-08-2026.\n";
        }
    }

    // ------------------------------------------------------------------
    // SETUP: (re)build the engine and queue from the objects in the DataStore
    // ------------------------------------------------------------------
    // Approved achievements are scored in file order; Pending ones join the
    // FIFO queue in file order; Rejected ones are just kept.
    void rebuild() {
        delete engine;
        delete verifier;
        engine = new RankEngine();
        verifier = new VerificationService();
        const DynArray<int>& ids = store.achievementIds();
        for (int i = 0; i < ids.size(); ++i) {
            Achievement* a = store.findAchievement(ids[i]);
            Student* s = store.findStudent(a->getStudentId());
            if (a->getStatus() == VerificationStatus::Approved) engine->applyAchievement(*s, *a);
            else if (a->getStatus() == VerificationStatus::Pending) verifier->submit(a);
        }
    }

    void generateFreshData() {
        store.clear();
        DataGenerator::generate(store, 100, 2026);
        rebuild();
    }

    // ------------------------------------------------------------------
    // OUTPUT HELPERS
    // ------------------------------------------------------------------
    void printLeaderboard(int k) const {
        DynArray<RankKey> top = engine->getTopK(k);
        std::cout << "\n======================= GEHU FORGE LEADERBOARD =======================\n";
        std::cout << std::left << std::setw(6) << "Rank" << std::setw(8) << "ID"
                  << std::setw(22) << "Name" << std::setw(10) << "EXP"
                  << std::setw(13) << "Tier" << "GitHub\n";
        std::cout << "----------------------------------------------------------------------\n";
        if (top.size() == 0) std::cout << "  (nobody is ranked yet)\n";
        for (int i = 0; i < top.size(); ++i) {
            const Student* s = store.findStudent(top[i].studentId);   // O(1) hash lookup
            std::cout << std::left << std::setw(6) << ("#" + std::to_string(i + 1))
                      << std::setw(8) << top[i].studentId
                      << std::setw(22) << (s ? s->getName() : "?")
                      << std::setw(10) << std::fixed << std::setprecision(2) << top[i].exp
                      << std::setw(13) << RankEngine::tierBadge(top[i].exp)
                      << (s ? s->getGithub() : "") << "\n";
        }
        std::cout << "======================================================================\n";
    }

    void showProfile(int id) {
        Student* s = store.findStudent(id);
        if (s == nullptr) { std::cout << "  ! No student with id " << id << ".\n"; return; }
        s->printProfileCard(engine->getRank(id), engine->getExp(id),
                            RankEngine::tierBadge(engine->getExp(id)));
    }

    void printStats() const {
        std::cout << "\n--- Dataset statistics ---\n"
                  << "Students            : " << store.studentCount() << "\n"
                  << "Achievements        : " << store.achievementCount() << "\n"
                  << "Ranked students     : " << engine->rankedCount() << "\n"
                  << "Waiting for review  : " << verifier->pendingCount() << "\n"
                  << "Leaderboard height  : " << engine->boardHeight()
                  << " (a perfectly balanced tree of " << engine->rankedCount()
                  << " nodes needs about log2(N) levels)\n";
    }

    // ------------------------------------------------------------------
    // MENU ACTIONS
    // ------------------------------------------------------------------
    void actionAddStudent() {
        std::string name, email, branch, github, leetcode, skillLine;
        int year = 0;
        if (!readText("Name: ", name)) return;
        while (true) {
            if (!readText("Email: ", email)) return;
            if (email.find('@') != std::string::npos) break;
            std::cout << "  ! An email must contain '@'.\n";
        }
        if (!readText("Branch (e.g. CSE): ", branch)) return;
        if (!readInt("Graduation year (2024-2035): ", 2024, 2035, year)) return;
        if (!readText("GitHub handle (optional): ", github, true)) return;
        if (!readText("LeetCode handle (optional): ", leetcode, true)) return;
        if (!readLine("Skills, comma separated (optional): ", skillLine)) return;

        Student* s = new Student(store.newStudentId(), name, email, branch, year);
        s->setGithub(github);
        s->setLeetcode(leetcode);
        DynArray<std::string> parts;
        splitLine(skillLine, ',', parts);
        for (int i = 0; i < parts.size(); ++i) {
            std::string skill = parts[i];
            size_t b = skill.find_first_not_of(' ');
            size_t e = skill.find_last_not_of(' ');
            if (b == std::string::npos || hasBadChar(skill)) continue;
            s->addSkill(skill.substr(b, e - b + 1));
        }
        if (!store.addStudent(s)) { delete s; std::cout << "  ! Could not add student.\n"; return; }
        unsavedChanges = true;
        std::cout << "  OK: student added with ID " << s->getStudentId() << ".\n";
    }

    void actionSubmitAchievement() {
        int studentId = 0, typeChoice = 0, score = 0;
        std::string title, date, proof, platform;
        if (!readInt("Student ID: ", 1, 999999, studentId)) return;
        if (store.findStudent(studentId) == nullptr) { std::cout << "  ! Unknown student id.\n"; return; }
        std::cout << "  1) Competitive programming  2) Certification  3) Open source  4) Internship\n";
        if (!readInt("Type (1-4): ", 1, 4, typeChoice)) return;
        if (!readText("Title: ", title)) return;
        if (!readDate("Date (DD-MM-YYYY): ", date)) return;
        if (!readInt("Raw score (-5000 to 100000): ", -5000, 100000, score)) return;
        if (!readText("Proof (link or file name): ", proof)) return;
        std::string type = typeChoice == 1 ? "CODING" : typeChoice == 2 ? "CERT" : typeChoice == 3 ? "OSS" : "INTERN";
        if (typeChoice == 1 && !readText("Platform (Codeforces/CodeChef/LeetCode/HackerRank/other): ", platform)) return;

        Achievement* a = createAchievement(type, store.newAchievementId(), studentId, title, date, score, proof, platform);
        if (a == nullptr || !store.addAchievement(a)) { delete a; std::cout << "  ! Could not add achievement.\n"; return; }
        verifier->submit(a);                                   // joins the FIFO queue as Pending
        unsavedChanges = true;
        std::cout << "  OK: achievement " << a->getId() << " is PENDING (position " << verifier->pendingCount()
                  << " in the review queue). It will not affect rankings until approved.\n";
    }

    void actionReviewNext() {
        int id = 0;
        if (!verifier->peekNextId(id)) { std::cout << "  The review queue is empty.\n"; return; }
        Achievement* a = store.findAchievement(id);
        std::cout << "\n--- Oldest submission in the queue (" << verifier->pendingCount() << " waiting) ---\n";
        if (a != nullptr) a->display();                        // polymorphic call
        std::string answer;
        while (true) {
            if (!readLine("Approve (a), Reject (r) or Skip (s)? ", answer)) return;
            if (answer == "a" || answer == "r" || answer == "s") break;
            std::cout << "  ! Type a, r or s.\n";
        }
        if (answer == "s") return;

        Achievement* reviewed = verifier->reviewNext(store.achievementRegistry(), answer == "a");
        if (reviewed == nullptr) { std::cout << "  ! Could not review.\n"; return; }
        unsavedChanges = true;
        if (reviewed->isApproved()) {
            Student* s = store.findStudent(reviewed->getStudentId());
            int before = engine->getRank(s->getStudentId());
            engine->applyAchievement(*s, *reviewed);
            std::cout << "  APPROVED. " << s->getName() << " now has " << std::fixed << std::setprecision(2)
                      << engine->getExp(s->getStudentId()) << " EXP, rank #" << engine->getRank(s->getStudentId())
                      << (before > 0 ? " (was #" + std::to_string(before) + ")" : " (newly ranked)") << ".\n";
        } else {
            std::cout << "  REJECTED. No EXP was added.\n";
        }
    }

    void actionSave() {
        if (store.saveToCsv(studentsPath, achievementsPath)) {
            unsavedChanges = false;
            std::cout << "  Saved to " << studentsPath << " and " << achievementsPath << ".\n";
        } else {
            std::cout << "  ! Could not write the files.\n";
        }
    }

    void printMenu() const {
        std::cout << "\n=============== GEHU FORGE ===============\n"
                  << " 1) Top 10 leaderboard\n 2) Top K leaderboard\n 3) Student profile\n"
                  << " 4) Add student\n 5) Submit achievement (goes to review queue)\n"
                  << " 6) Review next pending submission\n 7) Rank of a student\n"
                  << " 8) Dataset statistics\n 9) Save\n10) Reset to synthetic data\n 0) Save and exit\n"
                  << "(Pending reviews: " << verifier->pendingCount() << ")\n";
    }

public:
    App() : engine(nullptr), verifier(nullptr),
            studentsPath("data/students.csv"), achievementsPath("data/achievements.csv"),
            unsavedChanges(false) {
        engine = new RankEngine();
        verifier = new VerificationService();
    }

    App(const App&) = delete;
    App& operator=(const App&) = delete;

    ~App() {
        delete engine;
        delete verifier;
    }

    // ------------------------------------------------------------------
    // MAIN INTERACTIVE LOOP
    // ------------------------------------------------------------------
    int run() {
        std::filesystem::create_directories("data");
        if (store.loadFromCsv(studentsPath, achievementsPath)) {
            rebuild();
            std::cout << "Loaded " << store.studentCount() << " students and "
                      << store.achievementCount() << " achievements from data/.\n";
        } else {
            std::cout << "No data files found. Generating 100 synthetic students (seed 2026)...\n";
            generateFreshData();
            actionSave();
        }

        while (true) {
            printMenu();
            int choice = 0, id = 0, k = 0;
            if (!readInt("Choose: ", 0, 10, choice)) { actionSave(); break; }   // input ended

            if (choice == 0) { actionSave(); std::cout << "Goodbye!\n"; break; }
            else if (choice == 1) printLeaderboard(10);
            else if (choice == 2) {
                if (readInt("How many (1-100): ", 1, 100, k)) printLeaderboard(k);
            }
            else if (choice == 3) { if (readInt("Student ID: ", 1, 999999, id)) showProfile(id); }
            else if (choice == 4) actionAddStudent();
            else if (choice == 5) actionSubmitAchievement();
            else if (choice == 6) actionReviewNext();
            else if (choice == 7) {
                if (readInt("Student ID: ", 1, 999999, id)) {
                    if (store.findStudent(id) == nullptr) std::cout << "  ! Unknown student id.\n";
                    else if (engine->getRank(id) < 0) std::cout << "  Student " << id << " is unranked (no approved achievements).\n";
                    else std::cout << "  Student " << id << ": rank #" << engine->getRank(id) << " of "
                                   << engine->rankedCount() << ", EXP " << std::fixed << std::setprecision(2)
                                   << engine->getExp(id) << " (" << RankEngine::tierBadge(engine->getExp(id)) << ")\n";
                }
            }
            else if (choice == 8) printStats();
            else if (choice == 9) actionSave();
            else if (choice == 10) {
                std::string sure;
                if (readLine("This erases all your added data. Type 'yes' to continue: ", sure) && sure == "yes") {
                    generateFreshData();
                    actionSave();
                    std::cout << "  Reset done.\n";
                }
            }
        }
        return 0;
    }

    // ------------------------------------------------------------------
    // SCRIPTED DEMO (no typing needed, never touches the saved files)
    // ------------------------------------------------------------------
    int runDemo() {
        std::cout << "########## GEHU FORGE: SCRIPTED DEMO ##########\n";
        generateFreshData();
        printStats();
        std::cout << "\n[1] Top 10 of the synthetic dataset:";
        printLeaderboard(10);

        std::cout << "\n[2] A new student submits two achievements (they enter the FIFO queue as Pending).\n";
        Student* demo = new Student(store.newStudentId(), "Demo Student", "demo@example.edu", "CSE", 2027);
        demo->addSkill("C++"); demo->addSkill("DSA"); demo->addSkill("C++");   // duplicate ignored
        int demoId = demo->getStudentId();
        store.addStudent(demo);
        Achievement* big = createAchievement("INTERN", store.newAchievementId(), demoId, "Systems Intern",
                                             "01-09-2026", 3000, "offer_letter.pdf", "");
        store.addAchievement(big);
        verifier->submit(big);
        Achievement* weak = createAchievement("CERT", store.newAchievementId(), demoId, "Doubtful Certificate",
                                              "02-09-2026", 400, "unclear.pdf", "");
        store.addAchievement(weak);
        verifier->submit(weak);
        std::cout << "    Queue length now: " << verifier->pendingCount() << ". Demo Student rank: "
                  << engine->getRank(demoId) << " (-1 = unranked, pending items give no EXP)\n";

        std::cout << "\n[3] Reviewing in FIFO order (first 3 shown; older seeded items come first):\n";
        int shown = 0;
        while (verifier->hasPending()) {
            int nextId = 0;
            verifier->peekNextId(nextId);
            Achievement* a = verifier->reviewNext(store.achievementRegistry(), nextId != weak->getId());
            if (shown < 3) { std::cout << "    reviewed achievement #" << a->getId() << " -> "
                                       << Achievement::statusName(a->getStatus()) << "\n"; ++shown; }
            Student* owner = store.findStudent(a->getStudentId());
            if (a->isApproved()) engine->applyAchievement(*owner, *a);
        }
        std::cout << "    ... queue drained. 'Systems Intern' approved, 'Doubtful Certificate' rejected.\n";

        std::cout << "\n[4] Demo Student: rank #" << engine->getRank(demoId) << ", EXP "
                  << std::fixed << std::setprecision(2) << engine->getExp(demoId) << "\n";
        bool again = engine->applyAchievement(*demo, *big);
        std::cout << "[5] Trying to score the approved achievement a SECOND time: "
                  << (again ? "COUNTED (BUG!)" : "blocked (score-once guard)") << "\n";
        bool rej = engine->applyAchievement(*demo, *weak);
        std::cout << "[6] Trying to score the REJECTED achievement: "
                  << (rej ? "COUNTED (BUG!)" : "blocked (no EXP)") << "\n";

        std::cout << "\n[7] Top 10 after the review:";
        printLeaderboard(10);
        std::cout << "\n[8] Profile card:";
        showProfile(demoId);
        std::cout << "\nLeaderboard tree still balanced: " << (engine->boardIsBalanced() ? "yes" : "NO") << "\n";
        return 0;
    }
};
