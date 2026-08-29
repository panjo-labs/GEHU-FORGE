#pragma once // HEADER GUARD: Ensures single inclusion across compilation units

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <set>
#include <cmath>
#include <iomanip>
#include <algorithm>
#include <utility>
#include "User.h"
#include "Achievement.h"

// ============================================================================
// RANK ENGINE CORE (DSA: Hybrid Hash Map + Self-Balancing BST Architecture)
// ============================================================================
// Single Responsibility: Handles score normalization, anti-spam decay, 
// dynamic weighting, and leaderboard rankings for the GEU Forge platform.
class RankEngine {
private:
    // ------------------------------------------------------------------------
    // 1. DATA STRUCTURE: std::unordered_map<int, double> studentExpMap
    // ------------------------------------------------------------------------
    // WHY USED:
    // - Hash Table implementation mapping studentId -> current Normalized EXP.
    // - Time Complexity:  O(1) Average lookup, insert, and update.
    // - Space Complexity: O(N) where N is the total number of students.
    std::unordered_map<int, double> studentExpMap;

    // Internal Raw Score Tracker:
    // Tracks accumulated un-normalized weighted raw points before log scaling.
    // Time Complexity:  O(1) Average
    // Space Complexity: O(N)
    std::unordered_map<int, double> studentRawScoreMap;

    // ------------------------------------------------------------------------
    // 2. DATA STRUCTURE: std::set<std::pair<double, int>, std::greater<...>> dynamicLeaderboard
    // ------------------------------------------------------------------------
    // WHY USED:
    // - Self-Balancing Binary Search Tree (Red-Black Tree).
    // - Maintains (EXP, studentId) pairs in strict DESCENDING order.
    // - Guarantees fast real-time re-ranking without sorting the entire dataset:
    //    * Node Deletion: O(log N)
    //    * Node Insertion: O(log N)
    //    * Top-K Traversal: O(K)
    //    * Max EXP read: O(1) (*begin())
    // - Space Complexity: O(N) tree nodes.
    std::set<std::pair<double, int>, std::greater<std::pair<double, int>>> dynamicLeaderboard;

    // ------------------------------------------------------------------------
    // MATHEMATICAL SCORING ENGINE (Helper Functions)
    // ------------------------------------------------------------------------
    
    // Category Multiplier: Maps achievement domain to impact factor
    // Time Complexity:  O(1)
    // Space Complexity: O(1)
    double getCategoryWeight(AchievementCategory category) const {
        switch (category) {
            case AchievementCategory::INTERNSHIP:               return 5.0; // High impact industry experience
            case AchievementCategory::COMPETITIVE_PROGRAMMING:  return 3.5; // Core DSA & algorithmic proficiency
            case AchievementCategory::OPEN_SOURCE:              return 2.5; // Real-world collaborative dev
            case AchievementCategory::CERTIFICATION:            return 1.0; // Coursework & foundational certs
            case AchievementCategory::GENERAL:
            default:                                            return 1.0;
        }
    }

    // Platform Difficulty Multiplier: Normalizes score across contest platforms
    // Time Complexity:  O(P) where P is platform string length
    // Space Complexity: O(1) auxiliary
    double getPlatformMultiplier(std::string platform) const {
        std::transform(platform.begin(), platform.end(), platform.begin(), ::tolower);
        if (platform == "codeforces") return 2.5; // Highest algorithmic difficulty
        if (platform == "codechef")   return 1.8;
        if (platform == "leetcode")   return 1.2;
        if (platform == "hackerrank") return 0.8;
        return 1.0;                               // Default multiplier for unspecified platforms
    }

    // Dynamic Daily Activity Decay (Anti-Spam Algorithm):
    // Diminishing returns curve preventing exploitation from spamming low-effort PRs.
    // - Contributions 1-5:  1.0x (100% full points)
    // - Contributions 6-10: 0.4x (40% points)
    // - Contributions 11+:  0.1x (10% spam cap)
    // Time Complexity:  O(1)
    // Space Complexity: O(1)
    double getDailyActivityDecay(int dailyActivityCount) const {
        if (dailyActivityCount <= 5)  return 1.0;
        if (dailyActivityCount <= 10) return 0.4;
        return 0.1;
    }

    // Logarithmic Feature Scaling (Score Normalization):
    // Maps raw cumulative points smoothly to a bounded 0 - 10,000 EXP scale.
    // Formula: EXP = min(10000.0, 10000.0 * (ln(1 + rawScore) / ln(1 + 50000)))
    // WHY LOGARITHMIC:
    // Prevents runaway scores, rewards early consistency, and creates competitive tier bands.
    // Time Complexity:  O(1)
    // Space Complexity: O(1)
    double normalizeRawScore(double rawScore) const {
        if (rawScore <= 0.0) return 0.0;
        const double MAX_EXP = 10000.0;
        const double SATURATION_POINT = 50000.0; // Raw points needed for theoretical max 10,000 EXP
        double normalized = MAX_EXP * (std::log(1.0 + rawScore) / std::log(1.0 + SATURATION_POINT));
        return std::min(MAX_EXP, normalized);
    }

    // Assigns competitive tier badges based on normalized EXP milestones
    // Time Complexity:  O(1)
    // Space Complexity: O(1)
    std::string getTierBadge(double exp) const {
        if (exp >= 9000.0) return "Grandmaster [Diamond]";
        if (exp >= 7000.0) return "Master      [Platinum]";
        if (exp >= 4500.0) return "Expert      [Gold]";
        if (exp >= 2000.0) return "Specialist  [Silver]";
        return                    "Candidate   [Bronze]";
    }

public:
    RankEngine() = default;

    // ------------------------------------------------------------------------
    // API 1: PROCESS ACHIEVEMENT
    // ------------------------------------------------------------------------
    // Validates achievement, calculates weighted points, updates O(1) hash map,
    // and dynamically re-indexes the Red-Black Tree in O(log N).
    //
    // Time Complexity:
    // - Verification & Multipliers: O(1)
    // - BST Node Erase:            O(log N)
    // - Hash Map Update:           O(1) Average
    // - BST Node Insert:           O(log N)
    // - Total Time Complexity:     O(log N)
    //
    // Space Complexity: O(1) auxiliary memory per call.
    void processAchievement(Student* student, Achievement* ach, 
                            std::string platform = "", int dailyActivityCount = 1) {
        if (student == nullptr || ach == nullptr) {
            std::cerr << "[RankEngine Error]: Null student or achievement pointer." << std::endl;
            return;
        }

        // Integrity Check: Unverified achievements cannot affect rankings
        if (!ach->isVerified()) {
            std::cout << "[RankEngine Info]: Achievement ID " << ach->getId() 
                      << " is PENDING verification. EXP unchanged." << std::endl;
            return;
        }

        // Link achievement to student collection
        const auto& existingList = student->getAchievements();
        if (std::find(existingList.begin(), existingList.end(), ach) == existingList.end()) {
            student->addAchievement(ach);
        }

        int studentId = student->getStudentId();
        double baseScore = ach->getScore();
        double weightedDelta = 0.0;

        // Competitive Programming Rating Penalty:
        // Negative deltas are cushioned by 0.3x to prevent disproportionate demotions
        if (baseScore < 0.0) {
            weightedDelta = baseScore * 0.3;
        } else {
            double catWeight = getCategoryWeight(ach->getCategory());
            double platMult  = getPlatformMultiplier(platform);
            double actDecay  = getDailyActivityDecay(dailyActivityCount);
            weightedDelta = baseScore * catWeight * platMult * actDecay;
        }

        // Update raw points accumulator
        double currentRaw = studentRawScoreMap[studentId];
        double updatedRaw = std::max(0.0, currentRaw + weightedDelta);
        studentRawScoreMap[studentId] = updatedRaw;

        // Compute normalized EXP
        double newExp = normalizeRawScore(updatedRaw);

        // --------------------------------------------------------------------
        // DSA: BST RE-INDEXING (O(log N))
        // --------------------------------------------------------------------
        // Remove outdated node if student already existed in leaderboard
        if (studentExpMap.find(studentId) != studentExpMap.end()) {
            double oldExp = studentExpMap[studentId];
            dynamicLeaderboard.erase({oldExp, studentId}); // O(log N)
        }

        // Store updated EXP in O(1) hash map
        studentExpMap[studentId] = newExp;

        // Re-insert updated (EXP, studentId) node into Red-Black Tree
        dynamicLeaderboard.insert({newExp, studentId}); // O(log N)

        std::cout << "[RankEngine Success]: Processed '" << ach->getTitle() 
                  << "' for " << student->getName() 
                  << " | Delta: +" << std::fixed << std::setprecision(1) << weightedDelta
                  << " | EXP: " << std::fixed << std::setprecision(2) << newExp << std::endl;
    }

    // ------------------------------------------------------------------------
    // API 2: GET TOP-K STUDENTS
    // ------------------------------------------------------------------------
    // Traverses the top K nodes of the Balanced BST in descending order.
    //
    // Time Complexity:  O(K) where K is the requested number of top ranks.
    // Space Complexity: O(K) to store result pairs.
    std::vector<std::pair<int, double>> getTopK(int k) const {
        std::vector<std::pair<int, double>> topK;
        if (k <= 0) return topK;

        int count = 0;
        // In-order traversal starting from the root/max node
        for (auto it = dynamicLeaderboard.begin(); 
             it != dynamicLeaderboard.end() && count < k; ++it, ++count) {
            topK.push_back({it->second, it->first}); // {studentId, EXP}
        }
        return topK;
    }

    // ------------------------------------------------------------------------
    // API 3: GET STUDENT RANK POSITION
    // ------------------------------------------------------------------------
    // Computes the exact 1-indexed rank position of a given student.
    //
    // Time Complexity:  O(N) linear scan over BST nodes in worst case.
    // Space Complexity: O(1) auxiliary.
    int getStudentRank(int studentId) const {
        auto itMap = studentExpMap.find(studentId);
        if (itMap == studentExpMap.end()) return -1; // Not found

        double exp = itMap->second;
        int rank = 1;
        for (const auto& entry : dynamicLeaderboard) {
            if (entry.second == studentId && entry.first == exp) return rank;
            rank++;
        }
        return -1;
    }

    // Fast O(1) lookup for current student EXP
    // Time Complexity:  O(1) Average
    // Space Complexity: O(1)
    double getStudentExp(int studentId) const {
        auto it = studentExpMap.find(studentId);
        return (it != studentExpMap.end()) ? it->second : 0.0;
    }

    // ------------------------------------------------------------------------
    // API 4: DISPLAY LEADERBOARD TABLE
    // ------------------------------------------------------------------------
    // Generates a cleanly formatted text report of all ranked students.
    //
    // Time Complexity:  O(N) where N is number of ranked students.
    // Space Complexity: O(1) auxiliary.
    void displayLeaderboard(const std::unordered_map<int, Student*>& studentRegistry) const {
        std::cout << "\n=========================================================================================\n";
        std::cout << "                           GEU FORGE DYNAMIC LEADERBOARD                                 \n";
        std::cout << "=========================================================================================\n";
        std::cout << std::left 
                  << std::setw(6)  << "Rank"
                  << std::setw(12) << "Student ID"
                  << std::setw(20) << "Name"
                  << std::setw(12) << "EXP"
                  << std::setw(24) << "Tier / Status"
                  << std::setw(18) << "GitHub"
                  << std::endl;
        std::cout << "-----------------------------------------------------------------------------------------\n";

        if (dynamicLeaderboard.empty()) {
            std::cout << "                         No students currently ranked.                           \n";
        } else {
            int rank = 1;
            for (const auto& entry : dynamicLeaderboard) {
                double exp = entry.first;
                int id = entry.second;
                std::string name = "Unknown";
                std::string github = "N/A";

                auto regIt = studentRegistry.find(id);
                if (regIt != studentRegistry.end() && regIt->second != nullptr) {
                    name = regIt->second->getName();
                    if (!regIt->second->getGithub().empty()) {
                        github = regIt->second->getGithub();
                    }
                }

                std::cout << std::left 
                          << std::setw(6)  << ("#" + std::to_string(rank))
                          << std::setw(12) << id
                          << std::setw(20) << name
                          << std::setw(12) << std::fixed << std::setprecision(2) << exp
                          << std::setw(24) << getTierBadge(exp)
                          << std::setw(18) << github
                          << std::endl;
                rank++;
            }
        }
        std::cout << "=========================================================================================\n\n";
    }
};
