#pragma once // HEADER GUARD: Prevents duplicate definitions across multiple translation units

#include <iostream>
#include <string>
#include <vector>
#include <unordered_set>
#include "Achievement.h"

// ============================================================================
// BASE CLASS: User (OOP: Abstraction & Inheritance Base)
// ============================================================================
// Represents a generic system user. Provides foundational identity fields.
class User {
protected:
    // 'protected': Accessible within derived classes (like Student), inaccessible externally.
    std::string email;

public:
    // CONSTRUCTOR: Uses Member Initializer List for direct initialization (avoids extra copies)
    User(std::string email) : email(email) {}

    // VIRTUAL DESTRUCTOR (CRITICAL OOP CONCEPT):
    // Prevents memory leaks and undefined behavior when deleting a derived object 
    // through a base class pointer (e.g., User* u = new Student(...); delete u;).
    virtual ~User() {}

    // GETTER: Marked 'const' indicating it does not alter object state.
    // Time Complexity:  O(1)
    // Space Complexity: O(1)
    std::string getEmail() const { return email; }
};

// ============================================================================
// DERIVED CLASS: Student (OOP: Inheritance & Composition)
// ============================================================================
// Inherits public/protected members from User and manages specialized student data.
class Student : public User {
private:
    int studentId;          // Unique Primary Key used by RankEngine for O(1) hash lookups
    std::string name;
    std::string branch;
    int graduationYear;

    // ------------------------------------------------------------------------
    // DATA STRUCTURE: std::unordered_set<std::string>
    // ------------------------------------------------------------------------
    // WHY USED:
    // 1. Implemented as a Hash Table: Guarantees no duplicate skills are stored.
    // 2. Performance:
    //    - Insertion: O(1) average, O(N) worst-case (hash collision)
    //    - Search:    O(1) average
    //    - Deletion:  O(1) average
    // 3. Space Complexity: O(S) where S is the number of distinct skills.
    std::unordered_set<std::string> skills;

    std::string githubHandle;
    std::string leetcodeHandle;

    // ------------------------------------------------------------------------
    // DATA STRUCTURE: std::vector<Achievement*>
    // ------------------------------------------------------------------------
    // WHY USED:
    // 1. Dynamic contiguous array storing BASE CLASS POINTERS (Achievement*).
    // 2. Enables POLYMORPHISM (Heterogeneous collection storing CodingAchievement,
    //    CertificateAchievement, InternshipAchievement without Object Slicing).
    // 3. Performance:
    //    - Insertion (push_back): O(1) amortized time
    //    - Traversal:             O(A) where A is count of achievements (cache-friendly)
    // 4. Space Complexity: O(A) pointers.
    std::vector<Achievement*> achievements;

public:
    // CONSTRUCTOR:
    // Delegates email initialization to base User constructor via initializer list.
    Student(int id, std::string name, std::string email, std::string branch, int year)
        : User(email), studentId(id), name(name), branch(branch), graduationYear(year) {}

    // ------------------------------------------------------------------------
    // SKILL MANAGEMENT API
    // ------------------------------------------------------------------------
    // Time Complexity:  O(1) Average
    // Space Complexity: O(1) per unique skill added
    void addSkill(const std::string& skill) { skills.insert(skill); }

    // Time Complexity:  O(1) Average
    // Space Complexity: O(1)
    void removeSkill(const std::string& skill) { skills.erase(skill); }

    // ------------------------------------------------------------------------
    // SETTERS (Encapsulation)
    // ------------------------------------------------------------------------
    void setGithub(const std::string& github) { githubHandle = github; }
    void setLeetcode(const std::string& leetcode) { leetcodeHandle = leetcode; }

    // ------------------------------------------------------------------------
    // GETTERS
    // ------------------------------------------------------------------------
    int getStudentId() const { return studentId; }
    std::string getName() const { return name; }
    std::string getBranch() const { return branch; }
    int getGraduationYear() const { return graduationYear; }
    std::string getGithub() const { return githubHandle; }
    std::string getLeetcode() const { return leetcodeHandle; }

    // Returning 'const reference' avoids expensive deep copies of the hash set.
    // Time Complexity:  O(1)
    // Space Complexity: O(1) (No memory allocation)
    const std::unordered_set<std::string>& getSkills() const { return skills; }

    // ------------------------------------------------------------------------
    // ACHIEVEMENT INTEGRATION
    // ------------------------------------------------------------------------
    // Links an achievement to this student's profile.
    // Time Complexity:  O(1) amortized
    // Space Complexity: O(1)
    void addAchievement(Achievement* ach) {
        if (ach != nullptr) {
            achievements.push_back(ach);
        }
    }

    // Returns const reference to avoid vector copying
    // Time Complexity:  O(1)
    // Space Complexity: O(1)
    const std::vector<Achievement*>& getAchievements() const {
        return achievements;
    }

    // ------------------------------------------------------------------------
    // VIEW / PRESENTATION METHOD
    // ------------------------------------------------------------------------
    // Traverses skills and achievements for formatted profile display.
    // Time Complexity:  O(S + A) where S = skills count, A = achievements count
    // Space Complexity: O(1) auxiliary
    void displayProfile() const {
        std::cout << "\n========================================\n";
        std::cout << "           GEU FORGE PROFILE            \n";
        std::cout << "========================================\n";
        std::cout << "Student ID:      " << studentId << std::endl;
        std::cout << "Name:            " << name << std::endl;
        std::cout << "Email:           " << email << std::endl;
        std::cout << "Branch:          " << branch << std::endl;
        std::cout << "Graduation Year: " << graduationYear << std::endl;
        std::cout << "GitHub:          " << (githubHandle.empty() ? "N/A" : githubHandle) << std::endl;
        std::cout << "LeetCode:        " << (leetcodeHandle.empty() ? "N/A" : leetcodeHandle) << std::endl;
        std::cout << "Skills:          ";
        if (skills.empty()) {
            std::cout << "None listed";
        } else {
            for (const auto& skill : skills) {
                std::cout << "[" << skill << "] ";
            }
        }
        std::cout << "\nTotal Logged Achievements: " << achievements.size() << std::endl;
        std::cout << "========================================\n";
    }
};
