#pragma once // HEADER GUARD

#include <iostream>
#include <string>
#include <cctype>

// ============================================================================
// ENUMS  (enum class = strongly typed: you must write AchievementCategory::OPEN_SOURCE)
// ============================================================================
enum class AchievementCategory {
    INTERNSHIP,               // industry experience
    COMPETITIVE_PROGRAMMING,  // contests, problem solving
    OPEN_SOURCE,              // GitHub contributions
    CERTIFICATION,            // courses and exams
    GENERAL
};

// The life of a submission:   Pending --approve()--> Approved
//                             Pending --reject() --> Rejected
// Approved and Rejected are FINAL (no way back). This is a tiny "state machine".
enum class VerificationStatus { Pending, Approved, Rejected };

// ============================================================================
// ABSTRACT BASE CLASS: Achievement
// ============================================================================
// ABSTRACT = has at least one pure virtual function (= 0), so you can never
// create a plain Achievement. You can only create CodingAchievement,
// CertificateAchievement, etc. Code that handles any kind of achievement uses
// an 'Achievement*' and the right overridden function runs at run time
// (RUNTIME POLYMORPHISM).
class Achievement {
protected:
    // ENCAPSULATION: data is hidden. Outside code cannot set 'status' directly;
    // it must call approve() / reject(), which enforce the rules.
    int id;
    int studentId;                 // owner of this achievement
    std::string title;
    std::string date;              // "DD-MM-YYYY"
    int score;                     // raw base points claimed
    std::string proof;             // link or file name
    VerificationStatus status;     // starts as Pending
    bool pointsApplied;            // true once RankEngine has added the points
    AchievementCategory category;

    // Shared printing code for all derived display() functions.
    void printCommon() const {
        std::cout << "  ID: " << id << " | Student: " << studentId << "\n"
                  << "  Title: " << title << "\n"
                  << "  Date: " << date << " | Raw score: " << score << "\n"
                  << "  Proof: " << proof << "\n"
                  << "  Status: " << statusName(status) << "\n";
    }

public:
    // Constructor with a MEMBER INITIALIZER LIST (builds members directly, no extra copy).
    Achievement(int id, int studentId, const std::string& title, const std::string& date,
                int score, const std::string& proof, AchievementCategory category)
        : id(id), studentId(studentId), title(title), date(date), score(score),
          proof(proof), status(VerificationStatus::Pending), pointsApplied(false),
          category(category) {}

    // VIRTUAL DESTRUCTOR: DataStore deletes objects through Achievement* pointers.
    // Without 'virtual' only the base part would be destroyed (undefined behaviour).
    virtual ~Achievement() {}

    // ---- pure virtual: every concrete achievement MUST provide these ------
    virtual void display() const = 0;
    virtual double categoryWeight() const = 0;     // importance of this category
    virtual std::string typeName() const = 0;      // text used in CSV files

    // ---- virtual with a sensible default (derived classes MAY override) ---
    virtual double platformMultiplier() const { return 1.0; }
    virtual double basePoints() const { return static_cast<double>(score); }

    // TEMPLATE-METHOD style: fixed recipe, customised by the virtual parts above.
    // A negative score (e.g. contest rating loss) is cushioned to 30%.
    double weightedPoints() const {
        double base = basePoints();
        if (base < 0.0) return base * 0.3;
        return base * categoryWeight() * platformMultiplier();
    }

    // ---- state machine: only a Pending submission may change --------------
    bool approve() {
        if (status != VerificationStatus::Pending) return false;
        status = VerificationStatus::Approved;
        return true;
    }
    bool reject() {
        if (status != VerificationStatus::Pending) return false;
        status = VerificationStatus::Rejected;
        return true;
    }

    // SCORE-ONCE GUARD (check AND set in ONE call, so it cannot be forgotten):
    // returns true only the first time an Approved achievement asks to be scored.
    bool tryConsumeForScoring() {
        if (status != VerificationStatus::Approved || pointsApplied) return false;
        pointsApplied = true;
        return true;
    }

    // ---- getters (const = they never change the object) -------------------
    int getId() const { return id; }
    int getStudentId() const { return studentId; }
    const std::string& getTitle() const { return title; }
    const std::string& getDate() const { return date; }
    int getScore() const { return score; }
    const std::string& getProof() const { return proof; }
    VerificationStatus getStatus() const { return status; }
    bool isApproved() const { return status == VerificationStatus::Approved; }
    bool isPointsApplied() const { return pointsApplied; }
    AchievementCategory getCategory() const { return category; }

    static const char* statusName(VerificationStatus s) {
        switch (s) {
            case VerificationStatus::Pending:  return "Pending";
            case VerificationStatus::Approved: return "Approved";
            case VerificationStatus::Rejected: return "Rejected";
        }
        return "Unknown";
    }
};

// ============================================================================
// DERIVED CLASSES  (INHERITANCE: "a CodingAchievement IS-A Achievement")
// ============================================================================

// Competitive programming. Also knows its contest PLATFORM, which changes the points.
class CodingAchievement : public Achievement {
private:
    std::string platform;

    static std::string lower(std::string s) {
        for (size_t i = 0; i < s.size(); ++i)
            s[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(s[i])));
        return s;
    }

public:
    CodingAchievement(int id, int studentId, const std::string& title, const std::string& date,
                      int score, const std::string& proof, const std::string& platform)
        : Achievement(id, studentId, title, date, score, proof,
                      AchievementCategory::COMPETITIVE_PROGRAMMING),
          platform(platform) {}

    const std::string& getPlatform() const { return platform; }

    double categoryWeight() const override { return 3.5; }
    std::string typeName() const override { return "CODING"; }

    // OVERRIDE of the default: harder platforms are worth more.
    double platformMultiplier() const override {
        std::string p = lower(platform);
        if (p == "codeforces") return 2.5;
        if (p == "codechef")   return 1.8;
        if (p == "leetcode")   return 1.2;
        if (p == "hackerrank") return 0.8;
        return 1.0;
    }

    void display() const override {
        std::cout << "[Competitive Programming | " << platform << "]\n";
        printCommon();
    }
};

class CertificateAchievement : public Achievement {
public:
    CertificateAchievement(int id, int studentId, const std::string& title, const std::string& date,
                           int score, const std::string& proof)
        : Achievement(id, studentId, title, date, score, proof, AchievementCategory::CERTIFICATION) {}

    double categoryWeight() const override { return 1.0; }
    std::string typeName() const override { return "CERT"; }
    void display() const override { std::cout << "[Certification]\n"; printCommon(); }
};

class OpenSourceAchievement : public Achievement {
public:
    OpenSourceAchievement(int id, int studentId, const std::string& title, const std::string& date,
                          int score, const std::string& proof)
        : Achievement(id, studentId, title, date, score, proof, AchievementCategory::OPEN_SOURCE) {}

    double categoryWeight() const override { return 2.5; }
    std::string typeName() const override { return "OSS"; }
    void display() const override { std::cout << "[Open Source]\n"; printCommon(); }
};

class InternshipAchievement : public Achievement {
public:
    InternshipAchievement(int id, int studentId, const std::string& title, const std::string& date,
                          int score, const std::string& proof)
        : Achievement(id, studentId, title, date, score, proof, AchievementCategory::INTERNSHIP) {}

    double categoryWeight() const override { return 5.0; }
    std::string typeName() const override { return "INTERN"; }
    void display() const override { std::cout << "[Internship]\n"; printCommon(); }
};

// ============================================================================
// FACTORY FUNCTION
// ============================================================================
// Builds the right derived object from a text type ("CODING", "CERT", "OSS",
// "INTERN"). The CSV loader and the CLI both need this, so it lives in ONE place.
// Returns a NEW object (caller/DataStore owns it) or nullptr for unknown types.
inline Achievement* createAchievement(const std::string& type, int id, int studentId,
                                      const std::string& title, const std::string& date,
                                      int score, const std::string& proof,
                                      const std::string& platform) {
    if (type == "CODING") return new CodingAchievement(id, studentId, title, date, score, proof, platform);
    if (type == "CERT")   return new CertificateAchievement(id, studentId, title, date, score, proof);
    if (type == "OSS")    return new OpenSourceAchievement(id, studentId, title, date, score, proof);
    if (type == "INTERN") return new InternshipAchievement(id, studentId, title, date, score, proof);
    return nullptr;
}
