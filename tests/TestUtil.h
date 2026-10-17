#pragma once
// Tiny test helper: CHECK(condition) counts passes/failures and prints failures.
// Tests are the ONLY place where STL containers are allowed (as an independent "oracle").
#include <iostream>

static int g_checks = 0;
static int g_failed = 0;

#define CHECK(cond)                                                              \
    do {                                                                         \
        ++g_checks;                                                              \
        if (!(cond)) {                                                           \
            ++g_failed;                                                          \
            std::cout << "  FAIL  line " << __LINE__ << ": " #cond << "\n";      \
        }                                                                        \
    } while (0)

inline int finishTests(const char* name) {
    std::cout << name << ": " << (g_checks - g_failed) << "/" << g_checks << " checks passed\n";
    return g_failed == 0 ? 0 : 1;
}
