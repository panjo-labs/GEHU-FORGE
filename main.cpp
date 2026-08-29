#include <iostream>
#include <unordered_map>
#include <iomanip>
#include "Achievement.h"
#include "User.h"
#include "RankEngine.h"

using namespace std;

// ============================================================================
// SIMULATION DRIVER & VERIFICATION SUITE
// ============================================================================
// Demonstrates end-to-end execution flow: Student registration, achievement
// submission, verification checks, dynamic weighting, and leaderboard ranking.
int main() {
    cout << "=====================================================================\n";
    cout << "           GEU FORGE TALENT & RANKING SYSTEM SIMULATION              \n";
    cout << "=====================================================================\n\n";

    // ------------------------------------------------------------------------
    // STAGE 1: STUDENT REGISTRY SETUP
    // ------------------------------------------------------------------------
    // Data Structure: std::unordered_map<int, Student*>
    // Why used: Provides O(1) average-time student profile retrieval by studentId.
    // Space Complexity: O(N) where N is the number of enrolled students.
    unordered_map<int, Student*> studentRegistry;

    // Instantiating Student objects on the stack
    Student s1(1001, "Aarav Sharma", "aarav@geu.ac.in", "CSE", 2026);
    s1.setGithub("aarav-dev");
    s1.setLeetcode("aarav_cp");
    s1.addSkill("C++"); s1.addSkill("DSA"); s1.addSkill("System Design");

    Student s2(1002, "Diya Verma", "diya@geu.ac.in", "CSE", 2026);
    s2.setGithub("diya-oss");
    s2.addSkill("Python"); s2.addSkill("Open Source"); s2.addSkill("Kubernetes");

    Student s3(1003, "Rohan Gupta", "rohan@geu.ac.in", "IT", 2027);
    s3.setGithub("rohan-codes");
    s3.addSkill("Java"); s3.addSkill("Algorithms");

    Student s4(1004, "Sneha Patel", "sneha@geu.ac.in", "CSE", 2025);
    s4.setGithub("sneha-cloud");
    s4.addSkill("Cloud"); s4.addSkill("DevOps");

    // Registering pointers for O(1) lookup
    studentRegistry[s1.getStudentId()] = &s1;
    studentRegistry[s2.getStudentId()] = &s2;
    studentRegistry[s3.getStudentId()] = &s3;
    studentRegistry[s4.getStudentId()] = &s4;

    // ------------------------------------------------------------------------
    // STAGE 2: INITIALIZE RANKING ENGINE
    // ------------------------------------------------------------------------
    RankEngine engine;

    // ------------------------------------------------------------------------
    // STAGE 3: CREATE ACHIEVEMENTS (OOP: Polymorphism via Base Class)
    // ------------------------------------------------------------------------
    
    // Aarav: High-Tier Competitive Programming (Codeforces 2.5x) & Top Internship (5.0x)
    CodingAchievement ach1(1, "Div 1 Round #890 Top 50", "15-08-2026", 1200, "codeforces.com/contest/890");
    ach1.verify(); // Mark verified

    InternshipAchievement ach2(2, "SWE Intern @ Google", "01-06-2026", 3000, "internship_letter.pdf");
    ach2.verify();

    // Diya: Open Source Contributions (Testing Anti-Spam Activity Decay)
    OpenSourceAchievement ach3(3, "Merged Feature PR #402 in CNCF", "18-08-2026", 800, "github.com/cncf/pr/402");
    ach3.verify();

    OpenSourceAchievement ach4(4, "Daily Bugfixes (Activity Burst)", "19-08-2026", 500, "github.com/commits");
    ach4.verify();

    // Rohan: LeetCode Contest + Unverified Certificate
    CodingAchievement ach5(5, "LeetCode Biweekly Top 100", "20-08-2026", 600, "leetcode.com/contest");
    ach5.verify();

    CertificateAchievement ach6(6, "AWS Solutions Architect", "22-08-2026", 400, "aws_cert.pdf");
    // NOTE: ach6 is intentionally left UNVERIFIED to demonstrate security/verification gate

    // Sneha: Internship & Standard Certification
    InternshipAchievement ach7(7, "Cloud Engineering Intern @ Microsoft", "01-07-2026", 2800, "ms_letter.pdf");
    ach7.verify();

    CertificateAchievement ach8(8, "C++ Core Guidelines Cert", "10-08-2026", 350, "cpp_cert.pdf");
    ach8.verify();

    // ------------------------------------------------------------------------
    // STAGE 4: PROCESS ACHIEVEMENTS THROUGH RANK ENGINE
    // ------------------------------------------------------------------------
    // Time Complexity per call: O(log N) due to BST re-indexing
    cout << "--- PROCESSING CREDENTIALS ---\n";
    
    // Aarav: 1200 * 3.5 (CP) * 2.5 (Codeforces) = 10,500 raw; 3000 * 5.0 (Internship) = 15,000 raw
    engine.processAchievement(&s1, &ach1, "Codeforces", 1);
    engine.processAchievement(&s1, &ach2, "", 1);

    // Diya: Daily activity count = 8 triggers 0.4x anti-spam decay
    engine.processAchievement(&s2, &ach3, "GitHub", 2);
    engine.processAchievement(&s2, &ach4, "GitHub", 8);

    // Rohan: LeetCode Contest processed, unverified AWS cert rejected
    engine.processAchievement(&s3, &ach5, "LeetCode", 1);
    engine.processAchievement(&s3, &ach6, "", 1); // Rejected because isVerified() == false

    // Sneha
    engine.processAchievement(&s4, &ach7, "", 1);
    engine.processAchievement(&s4, &ach8, "", 1);

    // ------------------------------------------------------------------------
    // STAGE 5: DISPLAY LEADERBOARD (Time Complexity: O(N))
    // ------------------------------------------------------------------------
    engine.displayLeaderboard(studentRegistry);

    // ------------------------------------------------------------------------
    // STAGE 6: O(K) TOP-K TALENT RETRIEVAL
    // ------------------------------------------------------------------------
    cout << "--- TOP 3 STUDENTS (getTopK - O(K) Set Traversal) ---\n";
    auto top3 = engine.getTopK(3);
    for (size_t i = 0; i < top3.size(); ++i) {
        int id = top3[i].first;
        double exp = top3[i].second;
        cout << (i + 1) << ". " << studentRegistry[id]->getName() 
             << " (" << id << ") -> " << fixed << setprecision(2) << exp << " EXP\n";
    }

    // ------------------------------------------------------------------------
    // STAGE 7: DYNAMIC BST RE-INDEXING (O(log N) Update Upon Late Verification)
    // ------------------------------------------------------------------------
    cout << "\n--- LATE VERIFICATION & REAL-TIME RE-RANKING DEMO ---\n";
    cout << "Action: Admin verifies Rohan Gupta's AWS Solutions Architect Certificate...\n";
    ach6.verify(); // Approved!
    engine.processAchievement(&s3, &ach6, "", 1); // Dynamic O(log N) BST update

    cout << "\nUpdated Leaderboard after verification:";
    engine.displayLeaderboard(studentRegistry);

    return 0;
}
