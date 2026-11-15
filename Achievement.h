#pragma once

// Achievement ka abstract base class + uske derived types + factory.
// Runtime polymorphism: Achievement* se sahi override wala function chalta hai.

#include <iostream>
#include <string>
#include <cctype>

enum class AchievementCategory {
    INTERNSHIP,
    COMPETITIVE_PROGRAMMING,
    OPEN_SOURCE,
    CERTIFICATION,
    DSA,
    ACADEMIC,
    GENERAL
};

// Chhota state machine: Pending -> Approved ya Rejected. Dono FINAL hain.
enum class VerificationStatus { Pending, Approved, Rejected };

// Abstract class (pure virtual = 0), isliye seedha object nahi ban sakta.
class Achievement {
protected:
    // status bahar se direct set nahi hota - approve()/reject() se hi badlega.
    int id;
    int studentId;
    std::string title;
    std::string date;              // "DD-MM-YYYY"
    int score;
    std::string proof;
    VerificationStatus status;
    bool pointsApplied;            // RankEngine ne points jod diye toh true
    AchievementCategory category;

    void printCommon() const {
        std::cout << "  ID: " << id << " | Student: " << studentId << "\n"
                  << "  Title: " << title << "\n"
                  << "  Date: " << date << " | Raw score: " << score << "\n"
                  << "  Proof: " << proof << "\n"
                  << "  Status: " << statusName(status) << "\n";
    }

public:
    Achievement(int id, int studentId, const std::string& title, const std::string& date,
                int score, const std::string& proof, AchievementCategory category)
        : id(id), studentId(studentId), title(title), date(date), score(score),
          proof(proof), status(VerificationStatus::Pending), pointsApplied(false),
          category(category) {}

    // Virtual destructor zaroori hai - DataStore Achievement* se delete karta hai.
    virtual ~Achievement() {}

    virtual void display() const = 0;
    virtual double categoryWeight() const = 0;
    virtual std::string typeName() const = 0;      // CSV mein yahi text jata hai

    virtual double platformMultiplier() const { return 1.0; }
    virtual double basePoints() const { return static_cast<double>(score); }

    // Negative score (rating loss) ko 30% tak naram karo.
    double weightedPoints() const {
        double base = basePoints();
        if (base < 0.0) return base * 0.3;
        return base * categoryWeight() * platformMultiplier();
    }

    // Sirf Pending wala hi approve/reject ho sakta hai.
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

    // Score-once guard: check + set ek hi call mein. Pehli baar hi true milega.
    bool tryConsumeForScoring() {
        if (status != VerificationStatus::Approved || pointsApplied) return false;
        pointsApplied = true;
        return true;
    }

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

// Competitive programming - platform ke hisaab se points badalte hain.
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

    // Mushkil platform = zyada multiplier.
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

class DSAAchievement : public Achievement {
public:
    DSAAchievement(int id, int studentId, const std::string& title, const std::string& date,
                   int score, const std::string& proof)
        : Achievement(id, studentId, title, date, score, proof, AchievementCategory::DSA) {}

    double categoryWeight() const override { return 3.0; }
    std::string typeName() const override { return "DSA"; }
    void display() const override { std::cout << "[DSA]\n"; printCommon(); }
};

class AcademicAchievement : public Achievement {
public:
    AcademicAchievement(int id, int studentId, const std::string& title, const std::string& date,
                        int score, const std::string& proof)
        : Achievement(id, studentId, title, date, score, proof, AchievementCategory::ACADEMIC) {}

    double categoryWeight() const override { return 1.5; }
    std::string typeName() const override { return "ACAD"; }
    void display() const override { std::cout << "[Academic]\n"; printCommon(); }
};

// Factory: type text se sahi derived object banao. Naya object caller/DataStore own karta hai.
// Unknown type par nullptr.
inline Achievement* createAchievement(const std::string& type, int id, int studentId,
                                      const std::string& title, const std::string& date,
                                      int score, const std::string& proof,
                                      const std::string& platform) {
    if (type == "CODING") return new CodingAchievement(id, studentId, title, date, score, proof, platform);
    if (type == "CERT")   return new CertificateAchievement(id, studentId, title, date, score, proof);
    if (type == "OSS")    return new OpenSourceAchievement(id, studentId, title, date, score, proof);
    if (type == "INTERN") return new InternshipAchievement(id, studentId, title, date, score, proof);
    if (type == "DSA")    return new DSAAchievement(id, studentId, title, date, score, proof);
    if (type == "ACAD")   return new AcademicAchievement(id, studentId, title, date, score, proof);
    return nullptr;
}
