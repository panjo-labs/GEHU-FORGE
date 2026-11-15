#pragma once
#include <string>
#include "DynArray.h"
#include "HashMap.h"

// Trie - prefix search ke liye. Har token ke end node pe postings (studentId, weight).
// Insert O(L), prefix walk O(L) + subtree size.

struct Posting {
    int id;
    int weight;
    Posting() : id(0), weight(0) {}
    Posting(int i, int w) : id(i), weight(w) {}
};

class Trie {
private:
    static const int ALPHA = 36;   // a-z + 0-9

    struct Node {
        Node* child[ALPHA];
        DynArray<Posting> postings;
        Node() {
            for (int i = 0; i < ALPHA; ++i) child[i] = nullptr;
        }
    };

    Node* root;

    static int slot(char c) {
        if (c >= 'a' && c <= 'z') return c - 'a';
        if (c >= '0' && c <= '9') return 26 + (c - '0');
        return -1;
    }

    static void destroy(Node* n) {
        if (n == nullptr) return;
        for (int i = 0; i < ALPHA; ++i) destroy(n->child[i]);
        delete n;
    }

    // Subtree ke saare postings collect karo (DFS). exact node pe bonus milta hai.
    static void collect(const Node* n, bool exact, HashMap<int, int>& best) {
        for (int i = 0; i < n->postings.size(); ++i) {
            const Posting& p = n->postings[i];
            int score = exact ? p.weight * 2 : p.weight;
            int* cur = best.find(p.id);
            if (cur == nullptr || *cur < score) best.put(p.id, score);
        }
        for (int i = 0; i < ALPHA; ++i)
            if (n->child[i] != nullptr) collect(n->child[i], false, best);
    }

    const Node* walk(const std::string& word) const {
        const Node* cur = root;
        for (size_t i = 0; i < word.size() && cur != nullptr; ++i) {
            int s = slot(word[i]);
            if (s < 0) return nullptr;
            cur = cur->child[s];
        }
        return cur;
    }

public:
    Trie() : root(new Node()) {}
    ~Trie() { destroy(root); }
    Trie(const Trie&) = delete;
    Trie& operator=(const Trie&) = delete;

    void insert(const std::string& word, int id, int weight) {
        if (word.empty()) return;
        Node* cur = root;
        for (size_t i = 0; i < word.size(); ++i) {
            int s = slot(word[i]);
            if (s < 0) return;
            if (cur->child[s] == nullptr) cur->child[s] = new Node();
            cur = cur->child[s];
        }
        // Same student dobara aaye toh sirf max weight rakho
        for (int i = 0; i < cur->postings.size(); ++i) {
            if (cur->postings[i].id == id) {
                if (cur->postings[i].weight < weight) cur->postings[i].weight = weight;
                return;
            }
        }
        cur->postings.push_back(Posting(id, weight));
    }

    // prefix se shuru hone wale saare tokens ke students
    bool prefixSearch(const std::string& prefix, HashMap<int, int>& best) const {
        const Node* n = walk(prefix);
        if (n == nullptr) return false;
        collect(n, true, best);
        return true;
    }

    // Sirf exact token (fuzzy match ke baad use hota hai)
    void exactSearch(const std::string& word, int bonusDiv, HashMap<int, int>& best) const {
        const Node* n = walk(word);
        if (n == nullptr) return;
        for (int i = 0; i < n->postings.size(); ++i) {
            const Posting& p = n->postings[i];
            int score = p.weight / bonusDiv;
            if (score < 1) score = 1;
            int* cur = best.find(p.id);
            if (cur == nullptr || *cur < score) best.put(p.id, score);
        }
    }
};
