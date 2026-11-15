// Apne data structures ke unit tests.
// Build: g++ -std=c++17 -Wall -Wextra tests/test_structures.cpp -o test_structures.exe
#include <set>
#include <map>
#include <utility>
#include <string>
#include "TestUtil.h"
#include "../ds/DynArray.h"
#include "../ds/HashMap.h"
#include "../ds/HashSet.h"
#include "../ds/LinkedQueue.h"
#include "../ds/AVLTree.h"

static void testDynArray() {
    DynArray<int> a;
    CHECK(a.empty() && a.size() == 0 && a.capacity() == 0);
    for (int i = 0; i < 100; ++i) a.push_back(i * 2);
    CHECK(a.size() == 100);
    CHECK(a.capacity() >= 100);
    CHECK(a[0] == 0 && a[57] == 114 && a[99] == 198);
    DynArray<int> b(a);
    b[0] = 777;
    CHECK(a[0] == 0 && b[0] == 777);
    DynArray<int> c;
    c = a;
    DynArray<int>& same = c;
    c = same;                                 // a = a safe hona chahiye
    CHECK(c.size() == 100 && c[99] == 198);
    CHECK(a.removeAt(0) && a[0] == 2 && a.size() == 99);
    CHECK(!a.removeAt(500) && !a.removeAt(-1));
    a.clear();
    CHECK(a.empty());
}

static void testHashMap() {
    HashMap<int, int> m;
    CHECK(m.find(5) == nullptr && !m.contains(5) && !m.remove(5));
    for (int i = 0; i < 1000; ++i) m.put(i, i * 3);       // kai baar rehash hoga
    CHECK(m.size() == 1000);
    CHECK(m.bucketCount() > 16);
    CHECK(m.loadFactor() <= 0.75);
    bool allOk = true;
    for (int i = 0; i < 1000; ++i) {
        int* v = m.find(i);
        if (!v || *v != i * 3) allOk = false;
    }
    CHECK(allOk);
    m.put(7, -1);                                          // overwrite, size same
    CHECK(*m.find(7) == -1 && m.size() == 1000);
    CHECK(m.remove(7) && !m.contains(7) && m.size() == 999);
    int visited = 0;
    long sum = 0;
    m.forEach([&](const int& k, const int&) { ++visited; sum += k; });
    CHECK(visited == 999 && sum == 499500 - 7);

    HashMap<int, int> collide;                             // 16 ke multiples - same bucket
    for (int i = 0; i < 8; ++i) collide.put(i * 16, i);
    CHECK(collide.size() == 8 && *collide.find(48) == 3);
    CHECK(collide.remove(16) && collide.remove(0) && collide.size() == 6 && collide.contains(112));

    HashMap<std::string, int> sm;
    sm.put("alpha", 1);
    sm.put("beta", 2);
    sm.put("alpha", 3);
    CHECK(sm.size() == 2 && *sm.find("alpha") == 3 && sm.find("gamma") == nullptr);
}

static void testHashSet() {
    HashSet<std::string> s;
    CHECK(s.add("C++") && s.add("DSA") && !s.add("C++"));
    CHECK(s.size() == 2 && s.contains("DSA") && !s.contains("Java"));
    CHECK(s.remove("DSA") && !s.remove("DSA") && s.size() == 1);
    int n = 0;
    s.forEach([&](const std::string&) { ++n; });
    CHECK(n == 1);
}

static void testLinkedQueue() {
    LinkedQueue<int> q;
    int x = -1;
    CHECK(q.isEmpty() && !q.dequeue(x) && !q.peek(x) && x == -1);
    q.enqueue(10);
    q.enqueue(20);
    q.enqueue(30);
    CHECK(q.size() == 3 && q.peek(x) && x == 10);
    CHECK(q.dequeue(x) && x == 10);
    CHECK(q.dequeue(x) && x == 20);
    q.enqueue(40);
    CHECK(q.dequeue(x) && x == 30 && q.dequeue(x) && x == 40);
    CHECK(q.isEmpty() && !q.dequeue(x));
    q.enqueue(1);                                          // khaali hone ke baad bhi chalegi
    CHECK(q.size() == 1 && q.dequeue(x) && x == 1);
    LinkedQueue<int> leftover;                             // destructor ye nodes free karega
    for (int i = 0; i < 50; ++i) leftover.enqueue(i);
}

static bool lessInBoard(const std::pair<double, int>& a, const std::pair<double, int>& b) {
    if (a.first != b.first) return a.first > b.first;
    return a.second < b.second;
}

static void testAVL() {
    // sorted insert se plain BST stick ban jata; AVL chhota rehna chahiye
    AVLTree t;
    for (int i = 1; i <= 1000; ++i) t.insert(RankKey(i * 1.5, i));
    CHECK(t.size() == 1000 && t.isBalanced());
    CHECK(t.height() <= 14);                               // 1.44*log2(1000) ~ 14.4
    t.insert(RankKey(1.5, 1));
    CHECK(t.size() == 1000);

    // rotation cases LL, RR, LR, RL (har ek 3 keys)
    int orders[4][3] = {{3, 2, 1}, {1, 2, 3}, {3, 1, 2}, {1, 3, 2}};
    for (int c = 0; c < 4; ++c) {
        AVLTree r;
        for (int k = 0; k < 3; ++k) r.insert(RankKey(orders[c][k], orders[c][k]));
        CHECK(r.height() == 2 && r.isBalanced());
    }

    // std::set (oracle) se random comparison, rank + top-K bhi
    std::set<std::pair<double, int> > oracle;
    AVLTree tree;
    unsigned long seed = 12345;
    bool consistent = true;
    for (int step = 0; step < 4000; ++step) {
        seed = (seed * 1103515245UL + 12345UL) % 2147483648UL;
        int id = static_cast<int>((seed >> 8) % 300);
        seed = (seed * 1103515245UL + 12345UL) % 2147483648UL;
        double exp = static_cast<double>((seed >> 8) % 50) * 10.0;   // jaan-boojh ke ties
        seed = (seed * 1103515245UL + 12345UL) % 2147483648UL;
        bool doErase = ((seed >> 8) % 3) == 0;
        std::pair<double, int> key(exp, id);
        if (doErase) {
            bool o = oracle.erase(key) > 0;
            bool m = tree.erase(RankKey(exp, id));
            if (o != m) consistent = false;
        } else if (oracle.insert(key).second) {
            tree.insert(RankKey(exp, id));
        }
        if (step % 400 == 0 && !tree.isBalanced()) consistent = false;
    }
    CHECK(consistent);
    CHECK(tree.isBalanced());
    CHECK(tree.size() == static_cast<int>(oracle.size()));

    std::set<std::pair<double, int>, bool (*)(const std::pair<double, int>&, const std::pair<double, int>&)>
        ordered(lessInBoard);
    for (std::set<std::pair<double, int> >::const_iterator it = oracle.begin(); it != oracle.end(); ++it)
        ordered.insert(*it);
    std::map<int, std::pair<double, int> > expected;
    int rank = 1;
    for (auto it = ordered.begin(); it != ordered.end(); ++it) expected[rank++] = *it;

    bool ranksOk = true;
    for (auto it = expected.begin(); it != expected.end(); ++it) {
        if (tree.rankOf(RankKey(it->second.first, it->second.second)) != it->first) ranksOk = false;
    }
    CHECK(ranksOk);
    DynArray<RankKey> top;
    tree.topK(10, top);
    bool topOk = (top.size() == 10);
    for (int i = 0; topOk && i < 10; ++i) {
        if (top[i].exp != expected[i + 1].first || top[i].studentId != expected[i + 1].second) topOk = false;
    }
    CHECK(topOk);
    CHECK(tree.rankOf(RankKey(-1.0, -1)) == -1);
    tree.topK(100000, top);
    CHECK(top.size() == tree.size());
    tree.topK(0, top);
    CHECK(top.size() == 0);
}

int main() {
    testDynArray();
    testHashMap();
    testHashSet();
    testLinkedQueue();
    testAVL();
    return finishTests("test_structures");
}
