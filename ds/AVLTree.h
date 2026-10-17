#pragma once

#include "DynArray.h"

// ============================================================================
// RankKey  -  one leaderboard entry: (EXP, studentId)
// ============================================================================
struct RankKey {
    double exp;
    int    studentId;
    RankKey() : exp(0.0), studentId(0) {}
    RankKey(double e, int id) : exp(e), studentId(id) {}
};

// ORDERING RULE: "a comes before b" when a has MORE exp. If the exp is equal,
// the smaller studentId comes first. This is a strict, total order, so two
// different students can never be "equal" in the tree (ids are unique).
// Because the tree is sorted by this rule, an in-order walk gives rank 1 first.
inline bool before(const RankKey& a, const RankKey& b) {
    if (a.exp != b.exp) return a.exp > b.exp;
    return a.studentId < b.studentId;
}

inline bool sameKey(const RankKey& a, const RankKey& b) {
    return a.exp == b.exp && a.studentId == b.studentId;
}

// ============================================================================
// AVLTree  -  a self-balancing Binary Search Tree (replaces std::set)
// ============================================================================
// BST RULE: for every node, everything in its LEFT subtree comes before it and
// everything in its RIGHT subtree comes after it.
//
// PROBLEM WITH A PLAIN BST: inserting already-sorted data makes a long "stick"
// (height n), so every operation becomes O(n).
//
// AVL SOLUTION: keep the BALANCE FACTOR  (height(left) - height(right))  of every
// node in {-1, 0, +1}. When an insert/erase breaks that, fix it with ROTATIONS.
// Height stays about 1.44*log2(n), so insert / erase / search are O(log n).
//
// OUR EXTRA: every node also stores 'size' = number of nodes in its subtree.
// With sizes we can answer "what is the rank of this key?" in O(log n) by
// counting how many nodes lie to its left - no need to scan the whole tree.
//
// RULE OF THREE: the tree owns heap nodes -> destructor frees them all;
// copying is deleted.
class AVLTree {
private:
    struct Node {
        RankKey key;
        int     height;     // height of the subtree rooted here (leaf = 1)
        int     size;       // number of nodes in this subtree (leaf = 1)
        Node*   left;
        Node*   right;
        explicit Node(const RankKey& k)
            : key(k), height(1), size(1), left(nullptr), right(nullptr) {}
    };

    Node* root;

    // ---- small helpers (handle nullptr so callers stay simple) -------------
    static int heightOf(const Node* n) { return n ? n->height : 0; }
    static int sizeOf(const Node* n)   { return n ? n->size : 0; }
    static int maxInt(int a, int b)    { return a > b ? a : b; }

    // balance factor > 0  => left side is taller;  < 0 => right side is taller
    static int balanceOf(const Node* n) {
        return n ? heightOf(n->left) - heightOf(n->right) : 0;
    }

    // Recompute height and size of a node from its children.  O(1)
    static void update(Node* n) {
        n->height = 1 + maxInt(heightOf(n->left), heightOf(n->right));
        n->size   = 1 + sizeOf(n->left) + sizeOf(n->right);
    }

    // ---- rotations: local pointer changes that fix an imbalance ------------
    /*        y                x
             / \              / \
            x   C   --->     A   y        rotateRight(y)  (used for LL case)
           / \                  / \
          A   B                B   C                                      */
    static Node* rotateRight(Node* y) {
        Node* x = y->left;
        Node* b = x->right;
        x->right = y;       // y becomes the right child of x
        y->left  = b;       // subtree B moves under y
        update(y);          // y is lower now, so update it FIRST
        update(x);
        return x;           // x is the new root of this subtree
    }

    /*      x                  y
           / \                / \
          A   y      --->    x   C        rotateLeft(x)   (used for RR case)
             / \            / \
            B   C          A   B                                          */
    static Node* rotateLeft(Node* x) {
        Node* y = x->right;
        Node* b = y->left;
        y->left  = x;
        x->right = b;
        update(x);
        update(y);
        return y;
    }

    // Restore the AVL property at node n (after its subtree changed).
    // Four cases: LL, RR, LR, RL.
    static Node* rebalance(Node* n) {
        update(n);
        int bf = balanceOf(n);

        if (bf > 1) {                                   // left side too tall
            if (balanceOf(n->left) < 0)                 // LR case: first rotate child left
                n->left = rotateLeft(n->left);
            return rotateRight(n);                      // LL case (or LR after fix)
        }
        if (bf < -1) {                                  // right side too tall
            if (balanceOf(n->right) > 0)                // RL case: first rotate child right
                n->right = rotateRight(n->right);
            return rotateLeft(n);                       // RR case (or RL after fix)
        }
        return n;                                       // already balanced
    }

    // ---- recursive insert / erase -----------------------------------------
    static Node* insertNode(Node* n, const RankKey& k) {
        if (n == nullptr) return new Node(k);
        if (before(k, n->key))      n->left  = insertNode(n->left, k);
        else if (before(n->key, k)) n->right = insertNode(n->right, k);
        else return n;                                  // identical key: ignore
        return rebalance(n);
    }

    static Node* minNode(Node* n) {
        while (n->left != nullptr) n = n->left;
        return n;
    }

    static Node* eraseNode(Node* n, const RankKey& k, bool& removed) {
        if (n == nullptr) return nullptr;               // not found
        if (before(k, n->key)) {
            n->left = eraseNode(n->left, k, removed);
        } else if (before(n->key, k)) {
            n->right = eraseNode(n->right, k, removed);
        } else {                                        // found the node to delete
            removed = true;
            if (n->left == nullptr || n->right == nullptr) {
                Node* child = (n->left != nullptr) ? n->left : n->right;
                delete n;                               // 0 or 1 child: splice out
                return child;
            }
            // 2 children: copy the in-order successor's key here, then delete it below
            RankKey succKey = minNode(n->right)->key;   // COPY it: the node is deleted below
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

    // In-order walk that stops as soon as 'k' keys were collected.
    static void collect(const Node* n, int k, DynArray<RankKey>& out) {
        if (n == nullptr || out.size() >= k) return;
        collect(n->left, k, out);
        if (out.size() < k) out.push_back(n->key);
        collect(n->right, k, out);
    }

    // Verify heights, sizes, balance and ordering (used by the tests).
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

    // Insert a key.  O(log n)
    void insert(const RankKey& key) { root = insertNode(root, key); }

    // Remove a key. Returns false if it was not in the tree.  O(log n)
    bool erase(const RankKey& key) {
        bool removed = false;
        root = eraseNode(root, key, removed);
        return removed;
    }

    // 1-based rank of a key (1 = best), or -1 if the key is not in the tree.
    // Walk down from the root. Every time we go RIGHT we skip a whole left
    // subtree plus the node itself, so we add (left size + 1) to the rank.  O(log n)
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

    // The first k keys in leaderboard order.  O(k + log n)
    void topK(int k, DynArray<RankKey>& out) const {
        out.clear();
        if (k <= 0) return;
        collect(root, k, out);
    }

    int  size() const   { return sizeOf(root); }
    int  height() const { return heightOf(root); }

    // True when the tree satisfies every AVL rule (for testing).
    bool isBalanced() const {
        int h, s;
        return check(root, h, s);
    }
};
