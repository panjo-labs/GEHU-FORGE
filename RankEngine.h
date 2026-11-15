#pragma once

#include <string>
#include <cmath>
#include "User.h"
#include "Achievement.h"
#include "ds/HashMap.h"
#include "ds/AVLTree.h"
#include "ds/DynArray.h"

// RankEngine: Approved achievements ko EXP mein badlo + live leaderboard rakho.
// HashMap se purana EXP O(1) mein milta hai, AVLTree sorted rakhta hai (O(log N) update/rank).
class RankEngine {
private:
    HashMap<int, double> rawScore;            // studentId -> jude hue weighted points
    HashMap<int, double> expMap;              // studentId -> EXP (0..10000)
    HashMap<std::string, int> dailyCount;     // "studentId|date" -> us din ke items
    HashMap<int, double> multMap;             // studentId -> consistency multiplier (default 1.0)
    AVLTree board;

    // Ek din mein 5 tak full, 6-10 par 40%, uske baad 10%.
    static double dailyDecay(int countToday) {
        if (countToday <= 5)  return 1.0;
        if (countToday <= 10) return 0.4;
        return 0.1;
    }

    // Base EXP = 8000 * (1 - e^(-raw/6000)). Sirf achievements se max 8000 (Master mushkil),
    // baaki consistency multiplier (x1.25 tak) se aata hai. Shuru ke points zyada asar karte hain.
    static double normalize(double raw) {
        if (raw <= 0.0) return 0.0;
        return BASE_CAP * (1.0 - std::exp(-raw / CURVE));
    }

    double finalExp(int sid, double raw) const {
        const double* m = multMap.find(sid);
        double exp = normalize(raw) * (m != nullptr ? *m : 1.0);
        return exp < MAX_EXP ? exp : MAX_EXP;
    }

    // Tree mein purana node hatao, naya EXP daalo.  O(log N)
    void reKey(int sid, double newExp) {
        double* oldPtr = expMap.find(sid);
        if (oldPtr != nullptr) board.erase(RankKey(*oldPtr, sid));
        expMap.put(sid, newExp);
        board.insert(RankKey(newExp, sid));
    }

public:
    static constexpr double MAX_EXP = 10000.0;
    static constexpr double BASE_CAP = 8000.0;
    static constexpr double CURVE = 6000.0;

    static double baseExp(double raw) { return normalize(raw); }

    // Ek achievement ko score karo. Pending/Rejected/dobara aaye toh false.
    // appliedDelta mein woh points milte hain jo sach mein jude (category boards ke liye).
    bool applyAchievement(Student& student, Achievement& ach, double* appliedDelta = nullptr) {
        if (!ach.tryConsumeForScoring()) return false;
        student.linkAchievement(&ach);

        int sid = student.getStudentId();

        // Ek din mein zyada items = har extra item ki value kam (anti-spam)
        std::string dayKey = std::to_string(sid) + "|" + ach.getDate();
        int* dayPtr = dailyCount.find(dayKey);
        int countToday = (dayPtr != nullptr ? *dayPtr : 0) + 1;
        dailyCount.put(dayKey, countToday);

        double delta = ach.weightedPoints();
        if (delta > 0.0) delta *= dailyDecay(countToday);
        if (appliedDelta != nullptr) *appliedDelta = delta;

        addPoints(sid, delta);
        return true;
    }

    // Seedha raw points jodo (decay pehle hi lag chuka ho). O(log N)
    // Purana node hatao, naya EXP daalo - isliye map + tree dono chahiye.
    double addPoints(int sid, double delta) {
        double* rawPtr = rawScore.find(sid);
        double raw = (rawPtr != nullptr ? *rawPtr : 0.0) + delta;
        if (raw < 0.0) raw = 0.0;
        rawScore.put(sid, raw);

        double newExp = finalExp(sid, raw);
        reKey(sid, newExp);
        return newExp;
    }

    // Consistency multiplier set karo. Student ranked hai toh turant re-rank.  O(log N)
    void setMultiplier(int sid, double mult) {
        multMap.put(sid, mult);
        double* rawPtr = rawScore.find(sid);
        if (rawPtr != nullptr) reKey(sid, finalExp(sid, *rawPtr));
    }

    // Top k students, best pehle.  O(k + log N)
    DynArray<RankKey> getTopK(int k) const {
        DynArray<RankKey> result;
        board.topK(k, result);
        return result;
    }

    // 1-based rank; koi scored achievement nahi toh -1.  O(log N)
    int getRank(int studentId) const {
        const double* e = expMap.find(studentId);
        if (e == nullptr) return -1;
        return board.rankOf(RankKey(*e, studentId));
    }

    double getExp(int studentId) const {
        const double* e = expMap.find(studentId);
        return e != nullptr ? *e : 0.0;
    }

    int rankedCount() const { return board.size(); }
    int boardHeight() const { return board.height(); }

    bool boardIsBalanced() const { return board.isBalanced(); }

    static std::string tierBadge(double exp) {
        if (exp >= 9000.0) return "Grandmaster";
        if (exp >= 7000.0) return "Master";
        if (exp >= 4500.0) return "Expert";
        if (exp >= 2000.0) return "Specialist";
        return "Candidate";
    }
};
