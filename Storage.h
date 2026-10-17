#pragma once

#include <string>
#include <fstream>
#include <cstdlib>
#include "User.h"
#include "Achievement.h"
#include "ds/DynArray.h"
#include "ds/HashMap.h"
#include "ds/HashSet.h"

// ============================================================================
// splitLine  -  cut "a,b,c" into ["a","b","c"]   (used when reading CSV files)
// ============================================================================
inline void splitLine(const std::string& line, char delim, DynArray<std::string>& out) {
    out.clear();
    std::string current;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == delim) {
            out.push_back(current);
            current.clear();
        } else if (line[i] != '\r') {          // ignore Windows line endings
            current += line[i];
        }
    }
    out.push_back(current);                    // the last field has no delimiter after it
}

// Commas would break the CSV format, so free text is cleaned before saving.
inline std::string cleanText(std::string s) {
    for (size_t i = 0; i < s.size(); ++i) {
        if (s[i] == ',' || s[i] == ';' || s[i] == '\n' || s[i] == '\r') s[i] = ' ';
    }
    return s;
}

// ============================================================================
// DataStore  -  the single OWNER of every Student and Achievement object
// ============================================================================
// OWNERSHIP RULE: whoever calls 'new' must be answerable for 'delete'.
// DataStore deletes everything in its destructor, so nobody else deletes:
// Student, VerificationService and RankEngine only hold NON-OWNING pointers.
// (This idea is called RAII: the object cleans up after itself.)
//
// Two HashMaps give O(1) lookup by id. Two DynArrays remember insertion order,
// so the CSV files are always written in a stable, predictable order.
class DataStore {
private:
    HashMap<int, Student*> students;
    HashMap<int, Achievement*> achievements;
    DynArray<int> studentOrder;
    DynArray<int> achievementOrder;
    int nextStudentId;
    int nextAchievementId;

public:
    DataStore() : nextStudentId(2001), nextAchievementId(1) {}

    DataStore(const DataStore&) = delete;
    DataStore& operator=(const DataStore&) = delete;

    ~DataStore() { clear(); }

    // Delete every object and forget everything.  O(N + A)
    void clear() {
        for (int i = 0; i < studentOrder.size(); ++i) {
            Student** s = students.find(studentOrder[i]);
            if (s != nullptr) delete *s;
        }
        for (int i = 0; i < achievementOrder.size(); ++i) {
            Achievement** a = achievements.find(achievementOrder[i]);
            if (a != nullptr) delete *a;      // virtual destructor makes this safe
        }
        // Remove the (now dangling) pointers from the maps.
        for (int i = 0; i < studentOrder.size(); ++i) students.remove(studentOrder[i]);
        for (int i = 0; i < achievementOrder.size(); ++i) achievements.remove(achievementOrder[i]);
        studentOrder.clear();
        achievementOrder.clear();
        nextStudentId = 2001;
        nextAchievementId = 1;
    }

    // Take ownership of 's'. Returns false (and does NOT take it) if the id exists.
    bool addStudent(Student* s) {
        if (s == nullptr || students.contains(s->getStudentId())) return false;
        students.put(s->getStudentId(), s);
        studentOrder.push_back(s->getStudentId());
        if (s->getStudentId() >= nextStudentId) nextStudentId = s->getStudentId() + 1;
        return true;
    }

    // Take ownership of 'a'. Returns false if the id exists or the owner is unknown.
    bool addAchievement(Achievement* a) {
        if (a == nullptr || achievements.contains(a->getId())) return false;
        Student* owner = findStudent(a->getStudentId());
        if (owner == nullptr) return false;
        achievements.put(a->getId(), a);
        achievementOrder.push_back(a->getId());
        owner->linkAchievement(a);
        if (a->getId() >= nextAchievementId) nextAchievementId = a->getId() + 1;
        return true;
    }

    Student* findStudent(int id) {
        Student** s = students.find(id);
        return s != nullptr ? *s : nullptr;
    }
    Achievement* findAchievement(int id) {
        Achievement** a = achievements.find(id);
        return a != nullptr ? *a : nullptr;
    }
    // const versions: usable from const member functions such as saveToCsv()
    const Student* findStudent(int id) const {
        Student* const* s = students.find(id);
        return s != nullptr ? *s : nullptr;
    }
    const Achievement* findAchievement(int id) const {
        Achievement* const* a = achievements.find(id);
        return a != nullptr ? *a : nullptr;
    }

    int studentCount() const { return students.size(); }
    int achievementCount() const { return achievements.size(); }
    int newStudentId() { return nextStudentId; }
    int newAchievementId() { return nextAchievementId; }

    // Read-only access used by the verification service and the CLI.
    const HashMap<int, Achievement*>& achievementRegistry() const { return achievements; }
    const DynArray<int>& studentIds() const { return studentOrder; }
    const DynArray<int>& achievementIds() const { return achievementOrder; }

    // ---------------------------------------------------------------------
    // CSV SAVE.   O(N + A)
    // ---------------------------------------------------------------------
    bool saveToCsv(const std::string& studentsPath, const std::string& achievementsPath) const {
        std::ofstream sf(studentsPath.c_str());
        std::ofstream af(achievementsPath.c_str());
        if (!sf || !af) return false;

        sf << "id,name,email,branch,gradYear,github,leetcode,skills\n";
        for (int i = 0; i < studentOrder.size(); ++i) {
            const Student* s = findStudent(studentOrder[i]);
            std::string skillText;
            s->getSkills().forEach([&skillText](const std::string& skill) {
                if (!skillText.empty()) skillText += ";";
                skillText += cleanText(skill);
            });
            sf << s->getStudentId() << ',' << cleanText(s->getName()) << ','
               << cleanText(s->getEmail()) << ',' << cleanText(s->getBranch()) << ','
               << s->getGraduationYear() << ',' << cleanText(s->getGithub()) << ','
               << cleanText(s->getLeetcode()) << ',' << skillText << '\n';
        }

        af << "id,studentId,type,title,date,score,platform,proof,status\n";
        for (int i = 0; i < achievementOrder.size(); ++i) {
            const Achievement* a = findAchievement(achievementOrder[i]);
            std::string platform;
            // The platform exists only on CodingAchievement. Checking the type
            // name avoids a dynamic_cast.
            if (a->typeName() == "CODING") {
                platform = static_cast<const CodingAchievement*>(a)->getPlatform();
            }
            af << a->getId() << ',' << a->getStudentId() << ',' << a->typeName() << ','
               << cleanText(a->getTitle()) << ',' << a->getDate() << ',' << a->getScore() << ','
               << cleanText(platform) << ',' << cleanText(a->getProof()) << ','
               << Achievement::statusName(a->getStatus()) << '\n';
        }
        return true;
    }

    // ---------------------------------------------------------------------
    // CSV LOAD.  Returns false if a file cannot be opened.  Bad rows are skipped.
    // ---------------------------------------------------------------------
    bool loadFromCsv(const std::string& studentsPath, const std::string& achievementsPath) {
        std::ifstream sf(studentsPath.c_str());
        std::ifstream af(achievementsPath.c_str());
        if (!sf || !af) return false;
        clear();

        std::string line;
        DynArray<std::string> f;

        std::getline(sf, line);                         // skip the header row
        while (std::getline(sf, line)) {
            if (line.empty()) continue;
            splitLine(line, ',', f);
            if (f.size() < 8) continue;                 // malformed row
            Student* s = new Student(std::atoi(f[0].c_str()), f[1], f[2], f[3], std::atoi(f[4].c_str()));
            s->setGithub(f[5]);
            s->setLeetcode(f[6]);
            DynArray<std::string> skillList;
            splitLine(f[7], ';', skillList);
            for (int i = 0; i < skillList.size(); ++i) {
                if (!skillList[i].empty()) s->addSkill(skillList[i]);
            }
            if (!addStudent(s)) delete s;               // duplicate id: we still own it
        }

        std::getline(af, line);                         // skip the header row
        while (std::getline(af, line)) {
            if (line.empty()) continue;
            splitLine(line, ',', f);
            if (f.size() < 9) continue;
            Achievement* a = createAchievement(f[2], std::atoi(f[0].c_str()), std::atoi(f[1].c_str()),
                                               f[3], f[4], std::atoi(f[5].c_str()), f[7], f[6]);
            if (a == nullptr) continue;                 // unknown type
            if (f[8] == "Approved") a->approve();       // new objects start Pending
            else if (f[8] == "Rejected") a->reject();
            if (!addAchievement(a)) delete a;
        }
        return true;
    }
};

// ============================================================================
// DataGenerator  -  makes the synthetic (fake) demo dataset
// ============================================================================
// WHY OUR OWN RANDOM GENERATOR? std::rand() may give different numbers on
// different compilers. A Linear Congruential Generator (LCG) is a tiny formula:
//        state = (state * a + c) mod m
// With the same seed it ALWAYS produces the same sequence, so the same
// 100 students appear on every computer (reproducible results).
class DataGenerator {
private:
    unsigned long state;

    unsigned long nextRandom() {
        state = (state * 1103515245UL + 12345UL) % 2147483648UL;   // m = 2^31
        return state;
    }
    // A number in [0, n)
    int pick(int n) { return static_cast<int>((nextRandom() >> 8) % static_cast<unsigned long>(n)); }

    static std::string twoDigits(int v) { return (v < 10 ? "0" : "") + std::to_string(v); }

    explicit DataGenerator(unsigned seed) : state(seed) {}

    void fill(DataStore& store, int studentCount) {
        const char* firstNames[] = {"Aarav","Diya","Rohan","Sneha","Kabir","Ishita","Vihaan","Ananya",
                                    "Arjun","Meera","Dev","Riya","Yash","Tara","Nikhil","Pooja","Karan","Sana","Omkar","Neha"};
        const char* lastNames[]  = {"Sharma","Verma","Gupta","Patel","Rawat","Joshi","Negi","Bisht",
                                    "Kapoor","Mehta","Singh","Nair","Bhatt","Khanna","Saxena","Rana"};
        const char* branches[]   = {"CSE","IT","ECE","AIML","DS"};
        const char* skillPool[]  = {"C++","DSA","Python","Java","Web","SQL","Cloud","DevOps",
                                    "ML","Git","Linux","React","Embedded","Security"};
        const char* platforms[]  = {"Codeforces","CodeChef","LeetCode","HackerRank"};
        const char* codingTitles[] = {"Div2 Contest Top 500","Weekly Contest Top 100","Rated Round Finish","Starters Rank 50","Practice Milestone"};
        const char* certTitles[]   = {"Cloud Fundamentals Cert","Data Structures Course","Networking Cert","Python Specialisation","DBMS Certificate"};
        const char* ossTitles[]    = {"Merged Pull Request","Bug Fix Contribution","Docs Improvement","Feature Contribution","Issue Triage"};
        const char* internTitles[] = {"Software Intern","Data Intern","Web Dev Intern","Cloud Intern","Research Intern"};

        int achId = store.newAchievementId();
        for (int n = 0; n < studentCount; ++n) {
            int sid = 2001 + n;
            std::string first = firstNames[pick(20)];
            std::string last  = lastNames[pick(16)];
            std::string handle = first + "_" + last + std::to_string(sid);
            std::string email = first + "." + last + std::to_string(sid) + "@example.edu";
            for (size_t i = 0; i < email.size(); ++i)
                email[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(email[i])));

            Student* s = new Student(sid, first + " " + last, email, branches[pick(5)], 2026 + pick(3));
            s->setGithub("gh_" + handle);
            s->setLeetcode("lc_" + handle);
            int skillCount = 2 + pick(3);
            for (int k = 0; k < skillCount; ++k) s->addSkill(skillPool[pick(14)]);
            store.addStudent(s);

            int howMany = 1 + pick(3);
            for (int k = 0; k < howMany; ++k) {
                int kind = pick(10);
                std::string type, title, platform;
                int score;
                if (kind < 4)      { type = "CODING"; title = codingTitles[pick(5)]; platform = platforms[pick(4)]; score = 50 + pick(450); }
                else if (kind < 6) { type = "CERT";   title = certTitles[pick(5)];   score = 50 + pick(250); }
                else if (kind < 9) { type = "OSS";    title = ossTitles[pick(5)];    score = 50 + pick(350); }
                else               { type = "INTERN"; title = internTitles[pick(5)]; score = 300 + pick(700); }

                std::string date = twoDigits(1 + pick(28)) + "-" + twoDigits(1 + pick(12)) + "-2026";
                std::string proof = "proof_" + std::to_string(achId) + ".pdf";
                Achievement* a = createAchievement(type, achId, sid, title, date, score, proof, platform);

                int roll = pick(100);
                if (roll < 80) a->approve();
                else if (roll >= 92) a->reject();       // 80..91 stay Pending
                if (!store.addAchievement(a)) delete a;
                ++achId;
            }
        }
    }

public:
    // Fill 'store' with 'studentCount' fake students and 1-3 achievements each.
    // About 80% Approved, 12% Pending, 8% Rejected. Same seed => same data.
    static void generate(DataStore& store, int studentCount = 100, unsigned seed = 2026) {
        DataGenerator gen(seed);
        gen.fill(store, studentCount);
    }
};
