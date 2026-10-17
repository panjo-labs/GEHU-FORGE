#pragma once

#include <string>
#include <cmath>
#include "User.h"
#include "Achievement.h"
#include "ds/HashMap.h"
#include "ds/AVLTree.h"
#include "ds/DynArray.h"

// ============================================================================
// RankEngine  -  scoring maths + live leaderboard
// ============================================================================
// SINGLE RESPONSIBILITY: it turns APPROVED achievements into EXP and keeps the
// students sorted. It does not verify anything (VerificationService does) and
// does not print menus (App does).
//
// HYBRID DESIGN: two structures, each doing what it is best at.
//   HashMap  (expMap)  : studentId -> current EXP.     O(1) to find the OLD key.
//   AVLTree  (board)   : sorted (EXP, studentId) keys. O(log N) erase + insert,
//                        O(log N) rank, O(K) top-K.
// Without the map we would not know which key to erase from the tree.
// Without the tree we would have to sort all students again after every change.
//
// COMPOSITION: RankEngine HAS-A HashMap(s) and an AVLTree.
class RankEngine {
private:
    HashMap<int, double> rawScore;            // studentId -> accumulated weighted points
    HashMap<int, double> expMap;              // studentId -> normalised EXP (0..10000)
    HashMap<std::string, int> dailyCount;     // "studentId|date" -> approved items that day
    AVLTree board;                            // the leaderboard

    // Anti-spam curve: the more items a student gets scored on ONE day,
    // the less each extra item is worth.            O(1)
    static double dailyDecay(int countToday) {
        if (countToday <= 5)  return 1.0;     // items 1-5 : full credit
        if (countToday <= 10) return 0.4;     // items 6-10: 40%
        return 0.1;                           // items 11+ : 10%
    }

    // LOGARITHMIC NORMALISATION: raw points can grow without limit, EXP cannot.
    //   EXP = 10000 * ln(1 + raw) / ln(1 + 50000), capped at 10000.
    // Early points move EXP a lot, later points move it less (diminishing returns),
    // so one huge achievement cannot dominate the whole board.         O(1)
    static double normalize(double raw) {
        if (raw <= 0.0) return 0.0;
        const double MAX_EXP = 10000.0;
        const double SATURATION = 50000.0;    // raw points that reach MAX_EXP
        double exp = MAX_EXP * std::log(1.0 + raw) / std::log(1.0 + SATURATION);
        return exp < MAX_EXP ? exp : MAX_EXP;
    }

public:
    // Try to score one achievement for its student.
    // Returns false if it is not Approved, or was already scored (score-once guard).
    // Cost: O(1) hash work + O(log N) tree erase + O(log N) tree insert = O(log N).
    bool applyAchievement(Student& student, Achievement& ach) {
        if (!ach.tryConsumeForScoring()) return false;       // Pending/Rejected/duplicate
        student.linkAchievement(&ach);

        int sid = student.getStudentId();

        // 1) daily activity count for this student on this date
        std::string dayKey = std::to_string(sid) + "|" + ach.getDate();
        int* dayPtr = dailyCount.find(dayKey);
        int countToday = (dayPtr != nullptr ? *dayPtr : 0) + 1;
        dailyCount.put(dayKey, countToday);

        // 2) weighted points (category x platform) and anti-spam decay for gains
        double delta = ach.weightedPoints();
        if (delta > 0.0) delta *= dailyDecay(countToday);

        // 3) accumulate raw score (never below zero)
        double* rawPtr = rawScore.find(sid);
        double raw = (rawPtr != nullptr ? *rawPtr : 0.0) + delta;
        if (raw < 0.0) raw = 0.0;
        rawScore.put(sid, raw);

        // 4) new EXP, then re-index the leaderboard
        double newExp = normalize(raw);
        double* oldPtr = expMap.find(sid);
        if (oldPtr != nullptr) board.erase(RankKey(*oldPtr, sid));   // remove OLD node
        expMap.put(sid, newExp);
        board.insert(RankKey(newExp, sid));                          // add NEW node
        return true;
    }

    // First k students, best first.  O(k + log N)
    DynArray<RankKey> getTopK(int k) const {
        DynArray<RankKey> result;
        board.topK(k, result);
        return result;
    }

    // 1-based rank, or -1 if the student has no scored achievement.  O(log N)
    int getRank(int studentId) const {
        const double* e = expMap.find(studentId);
        if (e == nullptr) return -1;
        return board.rankOf(RankKey(*e, studentId));
    }

    // Current EXP (0.0 if unranked).  O(1)
    double getExp(int studentId) const {
        const double* e = expMap.find(studentId);
        return e != nullptr ? *e : 0.0;
    }

    int rankedCount() const { return board.size(); }
    int boardHeight() const { return board.height(); }

    // Debug/test helper: is the leaderboard tree still a valid AVL tree?
    bool boardIsBalanced() const { return board.isBalanced(); }

    // Badge shown next to the EXP.  O(1)
    static std::string tierBadge(double exp) {
        if (exp >= 9000.0) return "Grandmaster";
        if (exp >= 7000.0) return "Master";
        if (exp >= 4500.0) return "Expert";
        if (exp >= 2000.0) return "Specialist";
        return "Candidate";
    }
};
