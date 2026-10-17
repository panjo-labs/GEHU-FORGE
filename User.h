#pragma once

#include <iostream>
#include <string>
#include "Achievement.h"
#include "ds/DynArray.h"
#include "ds/HashSet.h"

// ============================================================================
// ABSTRACT BASE CLASS: User
// ============================================================================
// Holds what EVERY account has (name, email). It is abstract because of the
// pure virtual getRole(): you must create a concrete kind of user (Student).
class User {
protected:
    // 'protected': derived classes can use these, outside code cannot.
    std::string name;
    std::string email;

public:
    User(const std::string& name, const std::string& email) : name(name), email(email) {}

    // VIRTUAL DESTRUCTOR: safe deletion through a User* (see Achievement.h).
    virtual ~User() {}

    // Pure virtual: each kind of user reports its own role (polymorphism).
    virtual std::string getRole() const = 0;

    const std::string& getName() const  { return name; }
    const std::string& getEmail() const { return email; }
};

// ============================================================================
// DERIVED CLASS: Student   ("a Student IS-A User")
// ============================================================================
// COMPOSITION ("has-a"): a Student HAS-A HashSet of skills and HAS-A DynArray
// of achievements.
class Student : public User {
private:
    int studentId;                       // unique key used by every HashMap in the project
    std::string branch;
    int graduationYear;
    std::string githubHandle;
    std::string leetcodeHandle;

    HashSet<std::string> skills;         // duplicate-free, O(1) add/contains/remove

    // NON-OWNING pointers: the DataStore creates and deletes every Achievement.
    // Student only REMEMBERS them, so it must never call delete on them.
    DynArray<Achievement*> achievements;

public:
    // The initializer list calls the base-class constructor User(name, email).
    Student(int id, const std::string& name, const std::string& email,
            const std::string& branch, int graduationYear)
        : User(name, email), studentId(id), branch(branch), graduationYear(graduationYear) {}

    std::string getRole() const override { return "Student"; }

    // ---- skills ------------------------------------------------------------
    bool addSkill(const std::string& skill)       { return skills.add(skill); }
    bool removeSkill(const std::string& skill)    { return skills.remove(skill); }
    bool hasSkill(const std::string& skill) const { return skills.contains(skill); }
    // 'const reference' return: gives access WITHOUT copying the whole set.
    const HashSet<std::string>& getSkills() const { return skills; }

    // ---- simple getters / setters (ENCAPSULATION) ---------------------------
    int getStudentId() const { return studentId; }
    const std::string& getBranch() const { return branch; }
    int getGraduationYear() const { return graduationYear; }
    const std::string& getGithub() const { return githubHandle; }
    const std::string& getLeetcode() const { return leetcodeHandle; }
    void setGithub(const std::string& g) { githubHandle = g; }
    void setLeetcode(const std::string& l) { leetcodeHandle = l; }

    // ---- achievements -------------------------------------------------------
    // Remember an achievement. Ignores nullptr and ids already linked.  O(A)
    void linkAchievement(Achievement* a) {
        if (a == nullptr) return;
        for (int i = 0; i < achievements.size(); ++i) {
            if (achievements[i]->getId() == a->getId()) return;
        }
        achievements.push_back(a);
    }
    const DynArray<Achievement*>& getAchievements() const { return achievements; }

    // ---- profile card (view) -------------------------------------------------
    // Prints identity, skills and every achievement.  O(S + A)
    void printProfileCard(int rank, double exp, const std::string& tier) const {
        std::cout << "\n==================== GEHU FORGE PROFILE ====================\n";
        std::cout << "Student ID : " << studentId << "\n";
        std::cout << "Name       : " << name << "\n";
        std::cout << "Email      : " << email << "\n";
        std::cout << "Branch     : " << branch << "  (Batch " << graduationYear << ")\n";
        std::cout << "GitHub     : " << (githubHandle.empty() ? "N/A" : githubHandle) << "\n";
        std::cout << "LeetCode   : " << (leetcodeHandle.empty() ? "N/A" : leetcodeHandle) << "\n";

        std::cout << "Skills     : ";
        if (skills.size() == 0) {
            std::cout << "none listed";
        } else {
            skills.forEach([](const std::string& s) { std::cout << "[" << s << "] "; });
        }
        std::cout << "\n";

        if (rank > 0) {
            std::cout << "Rank       : #" << rank << "   EXP: " << exp << "   Tier: " << tier << "\n";
        } else {
            std::cout << "Rank       : unranked (no approved achievements yet)\n";
        }

        std::cout << "Achievements (" << achievements.size() << "):\n";
        for (int i = 0; i < achievements.size(); ++i) {
            // POLYMORPHISM: display() of the real derived type runs here.
            achievements[i]->display();
        }
        std::cout << "============================================================\n";
    }
};
