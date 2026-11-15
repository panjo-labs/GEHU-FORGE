// Search engine ke tests (Trie, MinHeap, edit distance, ranking).
// Build: g++ -std=c++17 -Wall -Wextra tests/test_search.cpp -o test_search.exe
#include <algorithm>
#include <cstdlib>
#include <vector>
#include "TestUtil.h"
#include "../ds/Trie.h"
#include "../ds/MinHeap.h"
#include "../SearchEngine.h"

struct IntLess {
    bool operator()(int a, int b) const { return a < b; }
};

static void testTrie() {
    Trie t;
    t.insert("python", 1, 4);
    t.insert("pytorch", 2, 4);
    t.insert("java", 3, 4);
    t.insert("python", 1, 6);
    HashMap<int, int> got;
    CHECK(t.prefixSearch("py", got));
    CHECK(got.size() == 2 && got.contains(1) && got.contains(2) && !got.contains(3));
    HashMap<int, int> exact;
    t.prefixSearch("python", exact);
    CHECK(exact.size() == 1 && *exact.find(1) == 12);   // exact match = weight x 2, max weight rakha
    HashMap<int, int> none;
    CHECK(!t.prefixSearch("rust", none) && none.size() == 0);
    CHECK(!t.prefixSearch("c++", none));                 // '+' trie mein nahi jaata
}

static void testHeap() {
    MinHeap<int, IntLess> h;
    std::vector<int> ref;
    std::srand(7);
    for (int i = 0; i < 2000; ++i) {
        int x = std::rand() % 5000;
        h.push(x);
        ref.push_back(x);
    }
    std::sort(ref.begin(), ref.end());
    bool ordered = true;
    for (size_t i = 0; i < ref.size(); ++i)
        if (h.pop() != ref[i]) ordered = false;
    CHECK(ordered);                                      // STL sort se match = heap sahi
    CHECK(h.empty());
}

static void testEditDistance() {
    CHECK(SearchEngine::editDistance("kitten", "sitting") == 3);
    CHECK(SearchEngine::editDistance("python", "pyhton") == 2);
    CHECK(SearchEngine::editDistance("", "abc") == 3);
    CHECK(SearchEngine::editDistance("same", "same") == 0);
}

static void testSearch() {
    SearchEngine se;
    se.addStudent(1, "Ananya Singh", "2412510001", "CSE", "Python;React", "Frontend developer", 5000);
    se.addStudent(2, "Anand Rawat", "2412510002", "IT", "C++;DSA", "Competitive programmer", 7000);
    se.addStudent(3, "Rohan Negi", "2412510003", "CSE", "Python;ML", "ML and data", 3000);
    se.addStudent(4, "Ritika Pant", "2412510004", "DS", "SQL", "Backend and APIs", 9000);

    DynArray<SearchHit> r;
    se.search("ana", 10, r);
    CHECK(r.size() == 2);                                // Ananya + Anand (prefix)
    se.search("python", 10, r);
    CHECK(r.size() == 2 && r[0].id == 1);                // same relevance -> zyada EXP pehle
    se.search("python ml", 10, r);
    CHECK(r.size() == 1 && r[0].id == 3);                // AND search
    se.search("c++", 10, r);
    CHECK(r.size() == 1 && r[0].id == 2);                // c++ -> cpp token
    se.search("pyhton", 10, r);
    CHECK(r.size() == 2);                                // typo bhi chalega
    se.search("2412510004", 10, r);
    CHECK(r.size() == 1 && r[0].id == 4);                // roll number
    se.search("ritika", 10, r);
    CHECK(r.size() == 1 && r[0].id == 4);
    se.search("zzzz", 10, r);
    CHECK(r.size() == 0);
    se.search("a", 1, r);
    CHECK(r.size() == 1);                                // top-K limit
}

int main() {
    testTrie();
    testHeap();
    testEditDistance();
    testSearch();
    return finishTests("search");
}
