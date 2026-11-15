#pragma once
#include <string>
#include "ds/DynArray.h"
#include "ds/HashMap.h"
#include "ds/HashSet.h"
#include "ds/Trie.h"
#include "ds/MinHeap.h"

// Student search engine:
//   Trie            -> prefix match ("ana" -> ananya, analytics)
//   Inverted index  -> har token ki postings Trie ke end node pe hi rakhi hain
//   Edit distance   -> typo ho toh bhi milega ("pyhton" -> python)
//   MinHeap top-K   -> N students mein se best K, O(N log K)
// Saare query words match hone chahiye (AND search).

struct SearchHit {
    int id;
    int relevance;
    double exp;
    SearchHit() : id(0), relevance(0), exp(0.0) {}
    SearchHit(int i, int r, double e) : id(i), relevance(r), exp(e) {}
};

// Heap ke liye "kaun chhota hai": kam relevance, phir kam EXP, phir bada id
struct WorseHit {
    bool operator()(const SearchHit& a, const SearchHit& b) const {
        if (a.relevance != b.relevance) return a.relevance < b.relevance;
        if (a.exp != b.exp) return a.exp < b.exp;
        return a.id > b.id;
    }
};

class SearchEngine {
private:
    Trie trie;
    HashSet<std::string> vocabSet;
    DynArray<std::string> vocab;
    HashMap<int, double> expOf;

    // Field weights: naam/roll sabse important, phir skills, phir baaki
    static const int W_NAME = 6;
    static const int W_ROLL = 6;
    static const int W_SKILL = 4;
    static const int W_OTHER = 2;

    static char lower(char c) { return (c >= 'A' && c <= 'Z') ? static_cast<char>(c - 'A' + 'a') : c; }

    // "C++" -> cpp, "C#" -> csharp, baaki non-alnum pe split
    static void tokenize(const std::string& text, DynArray<std::string>& out) {
        std::string t;
        for (size_t i = 0; i < text.size(); ++i) {
            char c = lower(text[i]);
            if (c == '+' && i + 1 < text.size() && text[i + 1] == '+') { t += "pp"; ++i; continue; }
            if (c == '#') { t += "sharp"; continue; }
            if ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9')) t += c;
            else if (!t.empty()) { out.push_back(t); t.clear(); }
        }
        if (!t.empty()) out.push_back(t);
    }

    void addField(const std::string& text, int id, int weight) {
        DynArray<std::string> toks;
        tokenize(text, toks);
        for (int i = 0; i < toks.size(); ++i) {
            trie.insert(toks[i], id, weight);
            if (vocabSet.add(toks[i])) vocab.push_back(toks[i]);
        }
    }

public:
    SearchEngine() {}
    SearchEngine(const SearchEngine&) = delete;
    SearchEngine& operator=(const SearchEngine&) = delete;

    // Levenshtein distance, DP do rows ke saath. O(m*n) time, O(n) memory.
    static int editDistance(const std::string& a, const std::string& b) {
        int m = static_cast<int>(a.size()), n = static_cast<int>(b.size());
        int* prev = new int[n + 1];
        int* cur = new int[n + 1];
        for (int j = 0; j <= n; ++j) prev[j] = j;
        for (int i = 1; i <= m; ++i) {
            cur[0] = i;
            for (int j = 1; j <= n; ++j) {
                int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
                int best = prev[j] + 1;
                if (cur[j - 1] + 1 < best) best = cur[j - 1] + 1;
                if (prev[j - 1] + cost < best) best = prev[j - 1] + cost;
                cur[j] = best;
            }
            int* t = prev; prev = cur; cur = t;
        }
        int d = prev[n];
        delete[] prev;
        delete[] cur;
        return d;
    }

    void addStudent(int id, const std::string& name, const std::string& roll,
                    const std::string& branch, const std::string& skills,
                    const std::string& headline, double exp) {
        addField(name, id, W_NAME);
        addField(roll, id, W_ROLL);
        addField(skills, id, W_SKILL);
        addField(branch, id, W_OTHER);
        addField(headline, id, W_OTHER);
        expOf.put(id, exp);
    }

    void setExp(int id, double exp) { expOf.put(id, exp); }
    int vocabSize() const { return vocab.size(); }

    // Best k results, sabse relevant pehle
    void search(const std::string& query, int k, DynArray<SearchHit>& out) const {
        out.clear();
        DynArray<std::string> qt;
        tokenize(query, qt);
        if (qt.size() == 0 || k <= 0) return;

        HashMap<int, int> total;     // id -> relevance sum
        HashMap<int, int> matched;   // id -> kitne query words match hue

        for (int t = 0; t < qt.size(); ++t) {
            HashMap<int, int> tok;
            trie.prefixSearch(qt[t], tok);

            // Prefix se kuch nahi mila -> typo maan ke nearest words dhoondo
            if (tok.size() == 0 && qt[t].size() >= 3) {
                int maxD = qt[t].size() >= 6 ? 2 : 1;
                for (int v = 0; v < vocab.size(); ++v) {
                    int lenDiff = static_cast<int>(vocab[v].size()) - static_cast<int>(qt[t].size());
                    if (lenDiff > maxD || -lenDiff > maxD) continue;
                    if (editDistance(qt[t], vocab[v]) <= maxD) trie.exactSearch(vocab[v], 2, tok);
                }
            }

            tok.forEach([&](const int& id, const int& score) {
                int* s = total.find(id);
                total.put(id, (s ? *s : 0) + score);
                int* c = matched.find(id);
                matched.put(id, (c ? *c : 0) + 1);
            });
        }

        // Top-K: heap mein sabse kharab result top pe, size k se zyada ho toh nikaal do
        MinHeap<SearchHit, WorseHit> heap;
        int need = qt.size();
        total.forEach([&](const int& id, const int& rel) {
            const int* c = matched.find(id);
            if (c == nullptr || *c < need) return;
            const double* e = expOf.find(id);
            heap.push(SearchHit(id, rel, e ? *e : 0.0));
            if (heap.size() > k) heap.pop();
        });

        DynArray<SearchHit> rev;
        while (!heap.empty()) rev.push_back(heap.pop());
        for (int i = rev.size() - 1; i >= 0; --i) out.push_back(rev[i]);
    }
};
