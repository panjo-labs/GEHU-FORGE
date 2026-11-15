#pragma once

#include "DynArray.h"

// AVLTree: leaderboard ke liye self-balancing BST (std::set ki jagah).
// insert/erase/rank O(log N), topK O(k + log N).

// Leaderboard ki ek entry: (EXP, studentId)
struct RankKey {
    double exp;
    int    studentId;
    RankKey() : exp(0.0), studentId(0) {}
    RankKey(double e, int id) : exp(e), studentId(id) {}
};

// Zyada EXP pehle; EXP barabar ho toh chhota studentId pehle.
// Ids unique hain, isliye do students kabhi "equal" nahi - in-order walk = rank 1 se.
inline bool before(const RankKey& a, const RankKey& b) {
    if (a.exp != b.exp) return a.exp > b.exp;
    return a.studentId < b.studentId;
}

inline bool sameKey(const RankKey& a, const RankKey& b) {
    return a.exp == b.exp && a.studentId == b.studentId;
}

// Har node ka balance factor {-1,0,+1} mein rakho, toota toh rotation se theek karo.
// Har node subtree 'size' bhi rakhta hai - isse rank O(log N) mein milta hai.
// Rule of Three: nodes hamare hain, destructor free karta hai, copy delete.
class AVLTree {
private:
    struct Node {
        RankKey key;
        int     height;     // leaf = 1
        int     size;       // subtree mein kitne nodes (leaf = 1)
        Node*   left;
        Node*   right;
        explicit Node(const RankKey& k)
            : key(k), height(1), size(1), left(nullptr), right(nullptr) {}
    };

    Node* root;

    static int heightOf(const Node* n) { return n ? n->height : 0; }
    static int sizeOf(const Node* n)   { return n ? n->size : 0; }
    static int maxInt(int a, int b)    { return a > b ? a : b; }

    // > 0 matlab left lamba, < 0 matlab right lamba
    static int balanceOf(const Node* n) {
        return n ? heightOf(n->left) - heightOf(n->right) : 0;
    }

    // Children se height + size dobara nikaalo.  O(1)
    static void update(Node* n) {
        n->height = 1 + maxInt(heightOf(n->left), heightOf(n->right));
        n->size   = 1 + sizeOf(n->left) + sizeOf(n->right);
    }

    // Rotation = sirf 2-3 pointer badlo. rotateRight LL case ke liye, rotateLeft RR ke liye.
    static Node* rotateRight(Node* y) {
        Node* x = y->left;
        Node* b = x->right;
        x->right = y;
        y->left  = b;
        update(y);          // y ab neeche hai, isliye pehle update
        update(x);
        return x;
    }

    static Node* rotateLeft(Node* x) {
        Node* y = x->right;
        Node* b = y->left;
        y->left  = x;
        x->right = b;
        update(x);
        update(y);
        return y;
    }

    // Node n par AVL property wapas lao. Chaar cases: LL, RR, LR, RL.
    static Node* rebalance(Node* n) {
        update(n);
        int bf = balanceOf(n);

        if (bf > 1) {
            if (balanceOf(n->left) < 0)                 // LR: pehle child ko left rotate
                n->left = rotateLeft(n->left);
            return rotateRight(n);                      // LL
        }
        if (bf < -1) {
            if (balanceOf(n->right) > 0)                // RL: pehle child ko right rotate
                n->right = rotateRight(n->right);
            return rotateLeft(n);                       // RR
        }
        return n;
    }

    static Node* insertNode(Node* n, const RankKey& k) {
        if (n == nullptr) return new Node(k);
        if (before(k, n->key))      n->left  = insertNode(n->left, k);
        else if (before(n->key, k)) n->right = insertNode(n->right, k);
        else return n;                                  // same key: ignore
        return rebalance(n);
    }

    static Node* minNode(Node* n) {
        while (n->left != nullptr) n = n->left;
        return n;
    }

    static Node* eraseNode(Node* n, const RankKey& k, bool& removed) {
        if (n == nullptr) return nullptr;
        if (before(k, n->key)) {
            n->left = eraseNode(n->left, k, removed);
        } else if (before(n->key, k)) {
            n->right = eraseNode(n->right, k, removed);
        } else {
            removed = true;
            if (n->left == nullptr || n->right == nullptr) {
                Node* child = (n->left != nullptr) ? n->left : n->right;
                delete n;                               // 0 ya 1 child: seedha hatao
                return child;
            }
            // 2 children: successor ki key yahan copy karo (copy, kyunki woh node neeche delete hoga).
            RankKey succKey = minNode(n->right)->key;
            n->key = succKey;
            bool dummy = false;
            n->right = eraseNode(n->right, succKey, dummy);
        }
        return rebalance(n);
    }

    static void destroy(Node* n) {
        if (n == nullptr) return;
        destroy(n->left);
        destroy(n->right);
        delete n;
    }

    // In-order walk, k keys milte hi ruk jao.
    static void collect(const Node* n, int k, DynArray<RankKey>& out) {
        if (n == nullptr || out.size() >= k) return;
        collect(n->left, k, out);
        if (out.size() < k) out.push_back(n->key);
        collect(n->right, k, out);
    }

    // Tests ke liye: height, size, balance aur order sab sahi hai?
    static bool check(const Node* n, int& h, int& s) {
        if (n == nullptr) { h = 0; s = 0; return true; }
        int lh, ls, rh, rs;
        if (!check(n->left, lh, ls) || !check(n->right, rh, rs)) return false;
        if (n->left  != nullptr && !before(n->left->key, n->key)) return false;
        if (n->right != nullptr && !before(n->key, n->right->key)) return false;
        h = 1 + maxInt(lh, rh);
        s = 1 + ls + rs;
        int bf = lh - rh;
        return h == n->height && s == n->size && bf >= -1 && bf <= 1;
    }

public:
    AVLTree() : root(nullptr) {}
    AVLTree(const AVLTree&) = delete;
    AVLTree& operator=(const AVLTree&) = delete;
    ~AVLTree() { destroy(root); }

    // O(log n)
    void insert(const RankKey& key) { root = insertNode(root, key); }

    // Key na mile toh false.  O(log n)
    bool erase(const RankKey& key) {
        bool removed = false;
        root = eraseNode(root, key, removed);
        return removed;
    }

    // 1-based rank (1 = best), na mile toh -1. Right jaate waqt (left size + 1) jodo.  O(log n)
    int rankOf(const RankKey& key) const {
        int rank = 0;
        const Node* cur = root;
        while (cur != nullptr) {
            if (sameKey(key, cur->key)) return rank + sizeOf(cur->left) + 1;
            if (before(key, cur->key)) {
                cur = cur->left;
            } else {
                rank += sizeOf(cur->left) + 1;
                cur = cur->right;
            }
        }
        return -1;
    }

    // Leaderboard order mein pehli k keys.  O(k + log n)
    void topK(int k, DynArray<RankKey>& out) const {
        out.clear();
        if (k <= 0) return;
        collect(root, k, out);
    }

    int  size() const   { return sizeOf(root); }
    int  height() const { return heightOf(root); }

    bool isBalanced() const {
        int h, s;
        return check(root, h, s);
    }
};
