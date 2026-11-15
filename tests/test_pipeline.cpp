// End-to-end tests: achievements, verification queue, rank engine aur storage ek saath.
// Build: g++ -std=c++17 -Wall -Wextra tests/test_pipeline.cpp -o test_pipeline.exe
#include <cmath>
#include <cstdio>
#include <string>
#include "TestUtil.h"
#include "../Achievement.h"
#include "../User.h"
#include "../Verification.h"
#include "../RankEngine.h"
#include "../Storage.h"

static bool near(double a, double b) { return std::fabs(a - b) < 0.01; }

static void testAchievementRules() {
    CodingAchievement cf(1, 10, "Round", "01-01-2026", 1200, "p", "Codeforces");
    CHECK(cf.getStatus() == VerificationStatus::Pending && !cf.isPointsApplied());
    CHECK(!cf.tryConsumeForScoring());                        // pending score nahi hoga
    CHECK(near(cf.weightedPoints(), 1200 * 3.5 * 2.5));
    CHECK(cf.approve() && cf.isApproved());
    CHECK(!cf.reject() && !cf.approve());                     // final state, wapas nahi
    CHECK(cf.tryConsumeForScoring());
    CHECK(!cf.tryConsumeForScoring());                        // dusri baar blocked (score-once)

    CodingAchievement bad(2, 10, "Rating loss", "01-01-2026", -1000, "p", "LeetCode");
    CHECK(near(bad.weightedPoints(), -300.0));                // 30% tak naram
    OpenSourceAchievement oss(3, 10, "PR", "01-01-2026", 100, "p");
    InternshipAchievement in(4, 10, "Intern", "01-01-2026", 100, "p");
    CertificateAchievement ce(5, 10, "Cert", "01-01-2026", 100, "p");
    CHECK(near(oss.weightedPoints(), 250) && near(in.weightedPoints(), 500) && near(ce.weightedPoints(), 100));
    CHECK(oss.reject() && !oss.isApproved());

    Achievement* viaFactory = createAchievement("OSS", 9, 10, "x", "01-01-2026", 10, "p", "");
    CHECK(viaFactory != nullptr && viaFactory->typeName() == "OSS");
    delete viaFactory;
    CHECK(createAchievement("BOGUS", 9, 10, "x", "01-01-2026", 10, "p", "") == nullptr);
}

static void testVerificationQueue() {
    HashMap<int, Achievement*> registry;
    CertificateAchievement a(1, 10, "A", "01-01-2026", 10, "p");
    CertificateAchievement b(2, 10, "B", "01-01-2026", 10, "p");
    CertificateAchievement c(3, 10, "C", "01-01-2026", 10, "p");
    registry.put(1, &a);
    registry.put(2, &b);
    registry.put(3, &c);
    VerificationService v;
    CHECK(!v.hasPending() && v.reviewNext(registry, true) == nullptr);
    CHECK(v.submit(&a) && v.submit(&b) && v.submit(&c));
    CHECK(v.pendingCount() == 3);
    int id = 0;
    CHECK(v.peekNextId(id) && id == 1);
    Achievement* r1 = v.reviewNext(registry, true);
    Achievement* r2 = v.reviewNext(registry, false);
    CHECK(r1 == &a && r1->isApproved());                       // FIFO: purana pehle
    CHECK(r2 == &b && r2->getStatus() == VerificationStatus::Rejected);
    CHECK(!v.submit(&a));
    CHECK(!v.submit(nullptr));
    CHECK(v.pendingCount() == 1);
}

static void testRankEngine() {
    RankEngine engine;
    Student s1(1, "One", "one@x.edu", "CSE", 2026);
    Student s2(2, "Two", "two@x.edu", "CSE", 2026);
    Student s3(3, "Three", "three@x.edu", "CSE", 2026);
    CHECK(s1.getRole() == "Student");

    InternshipAchievement big(1, 1, "Big", "01-01-2026", 3000, "p");
    CodingAchievement mid(2, 2, "Mid", "01-01-2026", 1200, "p", "Codeforces");
    CertificateAchievement small(3, 3, "Small", "01-01-2026", 400, "p");
    big.approve();
    mid.approve();                                             // 'small' Pending hi rahega

    CHECK(engine.getRank(1) == -1);
    CHECK(engine.applyAchievement(s1, big));
    CHECK(engine.applyAchievement(s2, mid));
    CHECK(!engine.applyAchievement(s3, small));
    CHECK(!engine.applyAchievement(s1, big));
    CHECK(engine.rankedCount() == 2 && engine.getRank(3) == -1);

    double expected = 10000.0 * std::log(15001.0) / std::log(50001.0);
    CHECK(near(engine.getExp(1), expected));
    CHECK(engine.getRank(1) == 1 && engine.getRank(2) == 2);

    // der se approve hua toh O(log N) mein re-rank
    small.approve();
    CHECK(engine.applyAchievement(s3, small));
    CHECK(engine.rankedCount() == 3 && engine.getRank(3) == 3);
    DynArray<RankKey> top = engine.getTopK(2);
    CHECK(top.size() == 2 && top[0].studentId == 1 && top[1].studentId == 2);
    CHECK(RankEngine::tierBadge(9500) == "Grandmaster" && RankEngine::tierBadge(100) == "Candidate");

    CertificateAchievement more[3] = {
        CertificateAchievement(10, 3, "m1", "02-01-2026", 9000, "p"),
        CertificateAchievement(11, 3, "m2", "03-01-2026", 9000, "p"),
        CertificateAchievement(12, 3, "m3", "04-01-2026", 9000, "p")};
    for (int i = 0; i < 3; ++i) { more[i].approve(); engine.applyAchievement(s3, more[i]); }
    CHECK(engine.getRank(3) == 1 && engine.getRank(1) == 2 && engine.getRank(2) == 3);
    CHECK(engine.boardIsBalanced());
}

static void testDailyDecay() {
    // Ek hi din 12 items: 1-5 full, 6-10 par 40%, 11-12 par 10%
    RankEngine engine;
    Student s(1, "Spam", "spam@x.edu", "CSE", 2026);
    DynArray<Achievement*> items;
    for (int i = 0; i < 12; ++i) {
        Achievement* a = new CertificateAchievement(i + 1, 1, "t", "05-05-2026", 100, "p");
        a->approve();
        items.push_back(a);
        engine.applyAchievement(s, *a);
    }
    double raw = 5 * 100.0 + 5 * 100.0 * 0.4 + 2 * 100.0 * 0.1;
    double expected = 10000.0 * std::log(1.0 + raw) / std::log(50001.0);
    CHECK(near(engine.getExp(1), expected));
    for (int i = 0; i < items.size(); ++i) delete items[i];
}

static void testStudentAndStore() {
    Student s(7, "Seven", "seven@x.edu", "IT", 2027);
    CHECK(s.addSkill("C++") && s.addSkill("DSA") && !s.addSkill("C++"));
    CHECK(s.hasSkill("DSA") && s.getSkills().size() == 2);
    CHECK(s.removeSkill("DSA") && !s.hasSkill("DSA"));
    CertificateAchievement a(1, 7, "t", "01-01-2026", 10, "p");
    s.linkAchievement(&a);
    s.linkAchievement(&a);
    s.linkAchievement(nullptr);
    CHECK(s.getAchievements().size() == 1);

    // generated data: sahi size, reproducible, CSV round-trip
    DataStore g1, g2;
    DataGenerator::generate(g1, 100, 2026);
    DataGenerator::generate(g2, 100, 2026);
    CHECK(g1.studentCount() == 100);
    CHECK(g1.achievementCount() >= 100 && g1.achievementCount() <= 300);
    CHECK(g1.achievementCount() == g2.achievementCount());
    CHECK(g1.findStudent(2001)->getName() == g2.findStudent(2001)->getName());
    CHECK(g1.findStudent(9999) == nullptr);

    const std::string sp = "test_students.tmp.csv", ap = "test_achievements.tmp.csv";
    CHECK(g1.saveToCsv(sp, ap));
    DataStore loaded;
    CHECK(loaded.loadFromCsv(sp, ap));
    CHECK(loaded.studentCount() == g1.studentCount() && loaded.achievementCount() == g1.achievementCount());
    bool same = true;
    for (int i = 0; i < g1.achievementIds().size(); ++i) {
        int id = g1.achievementIds()[i];
        const Achievement* x = g1.findAchievement(id);
        const Achievement* y = loaded.findAchievement(id);
        if (!y || x->getStatus() != y->getStatus() || x->getScore() != y->getScore() ||
            x->getTitle() != y->getTitle() || x->typeName() != y->typeName()) same = false;
    }
    CHECK(same);
    CHECK(!loaded.loadFromCsv("does_not_exist.csv", ap));
    std::remove(sp.c_str());
    std::remove(ap.c_str());

    // duplicate id reject (store owner nahi banta)
    DataStore store;
    Student* a1 = new Student(5, "A", "a@x.edu", "CSE", 2026);
    Student* a2 = new Student(5, "B", "b@x.edu", "CSE", 2026);
    CHECK(store.addStudent(a1) && !store.addStudent(a2));
    delete a2;
    Achievement* orphan = new CertificateAchievement(1, 999, "t", "01-01-2026", 1, "p");
    CHECK(!store.addAchievement(orphan));
    delete orphan;
}

static void testLargeScale() {
    // 5,000 students: rank consistent aur tree chhota rehna chahiye
    RankEngine engine;
    DataStore store;
    DataGenerator::generate(store, 5000, 7);
    for (int i = 0; i < store.achievementIds().size(); ++i) {
        Achievement* a = store.findAchievement(store.achievementIds()[i]);
        if (a->getStatus() == VerificationStatus::Approved) engine.applyAchievement(*store.findStudent(a->getStudentId()), *a);
    }
    CHECK(engine.rankedCount() > 4000);
    CHECK(engine.boardIsBalanced());
    CHECK(engine.boardHeight() <= 18);
    DynArray<RankKey> top = engine.getTopK(10);
    bool ordered = true;
    for (int i = 0; i < 10; ++i) {
        if (engine.getRank(top[i].studentId) != i + 1) ordered = false;
        if (i > 0 && top[i].exp > top[i - 1].exp) ordered = false;
    }
    CHECK(ordered);
}

int main() {
    testAchievementRules();
    testVerificationQueue();
    testRankEngine();
    testDailyDecay();
    testStudentAndStore();
    testLargeScale();
    return finishTests("test_pipeline");
}
