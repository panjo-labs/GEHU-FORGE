#pragma once

#include <iostream>
#include <string>
#include "Achievement.h"
#include "ds/DynArray.h"
#include "ds/HashSet.h"

// User (abstract base) aur Student (derived). Student ke paas skills ka HashSet
// aur achievements ka DynArray hai (composition).
class User {
protected:
    std::string name;
    std::string email;

public:
    User(const std::string& name, const std::string& email) : name(name), email(email) {}

    // User* se delete safe rahe isliye virtual destructor.
    virtual ~User() {}

    virtual std::string getRole() const = 0;

    const std::string& getName() const  { return name; }
    const std::string& getEmail() const { return email; }
};

class Student : public User {
private:
    int studentId;                       // har HashMap ki unique key
    std::string branch;
    int graduationYear;
    std::string githubHandle;
    std::string leetcodeHandle;

    HashSet<std::string> skills;         // duplicate nahi, O(1) add/contains/remove

    // Non-owning pointers: DataStore hi new/delete karta hai, Student kabhi delete nahi karega.
    DynArray<Achievement*> achievements;

public:
    Student(int id, const std::string& name, const std::string& email,
            const std::string& branch, int graduationYear)
        : User(name, email), studentId(id), branch(branch), graduationYear(graduationYear) {}

    std::string getRole() const override { return "Student"; }

    bool addSkill(const std::string& skill)       { return skills.add(skill); }
    bool removeSkill(const std::string& skill)    { return skills.remove(skill); }
    bool hasSkill(const std::string& skill) const { return skills.contains(skill); }
    const HashSet<std::string>& getSkills() const { return skills; }

    int getStudentId() const { return studentId; }
    const std::string& getBranch() const { return branch; }
    int getGraduationYear() const { return graduationYear; }
    const std::string& getGithub() const { return githubHandle; }
    const std::string& getLeetcode() const { return leetcodeHandle; }
    void setGithub(const std::string& g) { githubHandle = g; }
    void setLeetcode(const std::string& l) { leetcodeHandle = l; }

    // nullptr ya pehle se linked id ho toh ignore.  O(A)
    void linkAchievement(Achievement* a) {
        if (a == nullptr) return;
        for (int i = 0; i < achievements.size(); ++i) {
            if (achievements[i]->getId() == a->getId()) return;
        }
        achievements.push_back(a);
    }
    const DynArray<Achievement*>& getAchievements() const { return achievements; }

    // Profile, skills aur saare achievements print karo.  O(S + A)
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
            achievements[i]->display();
        }
        std::cout << "============================================================\n";
    }
};
