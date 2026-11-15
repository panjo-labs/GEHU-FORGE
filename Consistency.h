#pragma once

#include <cstdlib>
#include <string>
#include "ds/HashSet.h"

// Consistency bonus: roz platform kholna + har hafte kuch achieve karna = EXP multiplier (max x1.25).
// Din ko ek integer "day number" bana dete hain, taaki "kal" = day - 1 ho jaye.

// "YYYY-MM-DD" -> 1970-01-01 se din (civil calendar formula). Galat input = -1.  O(1)
inline int dayNumber(const std::string& iso) {
    if (iso.size() < 10 || iso[4] != '-' || iso[7] != '-') return -1;
    int y = std::atoi(iso.substr(0, 4).c_str());
    int m = std::atoi(iso.substr(5, 2).c_str());
    int d = std::atoi(iso.substr(8, 2).c_str());
    if (y < 1900 || m < 1 || m > 12 || d < 1 || d > 31) return -1;
    y -= m <= 2;
    int era = y / 400;
    int yoe = y - era * 400;
    int doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
    int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return era * 146097 + doe - 719468;
}

struct ConsistencyInfo {
    int loginStreak = 0;     // lagatar kitne din platform khola
    int activeDays30 = 0;    // pichhle 30 din mein kitne din aaya
    int weekStreak = 0;      // lagatar kitne hafte approved achievement
    double multiplier = 1.0;
};

class Consistency {
public:
    static constexpr int STREAK_CAP = 30;
    static constexpr int WEEK_CAP = 8;

    // ref = jis din ke hisaab se nikalna hai. Sab HashSet lookups, O(30 + 7*WEEK_CAP).
    static ConsistencyInfo at(const HashSet<int>& activeDays, const HashSet<int>& achievedDays, int ref) {
        ConsistencyInfo c;

        // Aaj nahi aaya toh kal se gino (din abhi khatam nahi hua)
        int d = activeDays.contains(ref) ? ref : ref - 1;
        while (c.loginStreak < STREAK_CAP && activeDays.contains(d)) { ++c.loginStreak; --d; }

        for (int i = 0; i < 30; ++i)
            if (activeDays.contains(ref - i)) ++c.activeDays30;

        // Hafta w = [ref-7w-6 .. ref-7w]. Is hafte kuch nahi toh pichhle se shuru
        int w = weekHas(achievedDays, ref, 0) ? 0 : 1;
        while (c.weekStreak < WEEK_CAP && weekHas(achievedDays, ref, w)) { ++c.weekStreak; ++w; }

        c.multiplier = 1.0
            + 0.10 * c.activeDays30 / 30.0
            + 0.05 * c.loginStreak / (double)STREAK_CAP
            + 0.10 * c.weekStreak / (double)WEEK_CAP;
        return c;
    }

private:
    static bool weekHas(const HashSet<int>& days, int ref, int w) {
        int end = ref - 7 * w;
        for (int i = 0; i < 7; ++i)
            if (days.contains(end - i)) return true;
        return false;
    }
};
