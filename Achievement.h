#pragma once // HEADER GUARD: Prevents multiple inclusions of this file across the project

#include <iostream>
#include <string>

// ============================================================================
// ENUM: AchievementCategory
// ============================================================================
// Strongly typed enum used by RankEngine to apply domain-specific multipliers
enum class AchievementCategory {
    INTERNSHIP,               // 5.0x multiplier (Industry experience)
    COMPETITIVE_PROGRAMMING,  // 3.5x multiplier (Problem solving & DSA)
    OPEN_SOURCE,              // 2.5x multiplier (Real-world software dev)
    CERTIFICATION,            // 1.0x multiplier (Coursework & exams)
    GENERAL                   // 1.0x default
};

// ============================================================================
// BASE CLASS: Achievement (OOP: Encapsulation & Abstraction)
// ============================================================================
// Blueprint defining common attributes and behaviors for all achievements.
class Achievement {
protected:
    // 'protected': Accessible by derived classes, hidden from outside code (Encapsulation)
    int id;                        // Unique identifier for the achievement
    std::string title;             // Name/title of the achievement
    std::string date;              // Date earned or submitted
    int score;                     // Raw base score / rating points
    std::string proof;             // URL, file link, or certificate artifact
    bool verified;                 // Verification status (true = approved, false = pending)
    AchievementCategory category;  // Category tag for dynamic weight calculations

public:
    // CONSTRUCTOR: Uses member initializer list for efficient attribute initialization
    Achievement(int id, std::string title, std::string date, int score, std::string proof,
                AchievementCategory category = AchievementCategory::GENERAL)
        : id(id), title(title), date(date), score(score), proof(proof),
          verified(false), category(category) {}

    // VIRTUAL DESTRUCTOR: Ensures derived class destructors are called when deleted via Achievement*
    virtual ~Achievement() {}

    // OOP: POLYMORPHISM (Virtual Function)
    // 'virtual' allows derived classes to override this method at runtime (dynamic binding)
    virtual void display() const {
        std::cout << "ID:        " << id << std::endl;
        std::cout << "Title:     " << title << std::endl;
        std::cout << "Date:      " << date << std::endl;
        std::cout << "Raw Score: " << score << std::endl;
        std::cout << "Proof:     " << proof << std::endl;
        std::cout << "Status:    " << (verified ? "Verified" : "Pending") << std::endl;
    }

    // MUTATOR: Marks the achievement as verified/approved
    void verify() { verified = true; }

    // GETTERS: Marked 'const' because they only read data without modifying object state
    int getId() const { return id; }
    int getScore() const { return score; }
    bool isVerified() const { return verified; }
    std::string getTitle() const { return title; }
    AchievementCategory getCategory() const { return category; }
};

// ============================================================================
// DERIVED CLASS: CodingAchievement (OOP: Inheritance)
// ============================================================================
// Represents Competitive Programming & DSA milestones
class CodingAchievement : public Achievement {
public:
    // Constructor delegates member initialization to parent Achievement constructor
    CodingAchievement(int id, std::string title, std::string date, int score, std::string proof)
        : Achievement(id, title, date, score, proof, AchievementCategory::COMPETITIVE_PROGRAMMING) {}

    // OVERRIDE: Customizes display specifically for Coding achievements
    void display() const override {
        std::cout << "[Category: Competitive Programming / Coding]" << std::endl;
        Achievement::display(); // Reuses base class printing logic
    }
};

// ============================================================================
// DERIVED CLASS: CertificateAchievement (OOP: Inheritance)
// ============================================================================
// Represents formal credentials & course certificates
class CertificateAchievement : public Achievement {
public:
    CertificateAchievement(int id, std::string title, std::string date, int score, std::string proof)
        : Achievement(id, title, date, score, proof, AchievementCategory::CERTIFICATION) {}

    void display() const override {
        std::cout << "[Category: Certification]" << std::endl;
        Achievement::display();
    }
};

// ============================================================================
// DERIVED CLASS: OpenSourceAchievement (OOP: Inheritance)
// ============================================================================
// Represents GitHub PRs, commits, and open-source contributions
class OpenSourceAchievement : public Achievement {
public:
    OpenSourceAchievement(int id, std::string title, std::string date, int score, std::string proof)
        : Achievement(id, title, date, score, proof, AchievementCategory::OPEN_SOURCE) {}

    void display() const override {
        std::cout << "[Category: Open Source / Dev]" << std::endl;
        Achievement::display();
    }
};

// ============================================================================
// DERIVED CLASS: InternshipAchievement (OOP: Inheritance)
// ============================================================================
// Represents industry internships and real-world work experience
class InternshipAchievement : public Achievement {
public:
    InternshipAchievement(int id, std::string title, std::string date, int score, std::string proof)
        : Achievement(id, title, date, score, proof, AchievementCategory::INTERNSHIP) {}

    void display() const override {
        std::cout << "[Category: Internship / Industry]" << std::endl;
        Achievement::display();
    }
};
