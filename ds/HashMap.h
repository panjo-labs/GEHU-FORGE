#pragma once

#include <string>

// ============================================================================
// HASH FUNCTIONS
// ============================================================================
// A hash function turns a key into a big number. We later do  number % buckets
// to decide which "bucket" (slot) the key lives in.

// For integers (student ids, achievement ids) the number itself is a fine hash.
inline unsigned long hashKey(int key) {
    return static_cast<unsigned long>(key < 0 ? -key : key);
}

// For strings we use a "polynomial rolling hash":  h = h * 31 + character.
// It mixes every character, so "abc" and "acb" get different numbers.
inline unsigned long hashKey(const std::string& key) {
    unsigned long h = 0;
    for (size_t i = 0; i < key.size(); ++i) {
        h = h * 31 + static_cast<unsigned char>(key[i]);
    }
    return h;
}

// ============================================================================
// HashMap<K, V>  -  our own hash table (replaces std::unordered_map)
// ============================================================================
// STRUCTURE: an array of buckets. Each bucket is a linked list ("chain") of
// nodes. This is called SEPARATE CHAINING:
//
//    bucket 0 : -> [id 16] -> [id 32]      (two keys collided here)
//    bucket 1 : -> [id 1]
//    bucket 2 : (empty)
//    bucket 3 : -> [id 19]
//
// put / find / remove: compute hash, jump to ONE bucket, walk its short chain.
//   Average O(1). Worst case O(n) if every key lands in the same bucket.
//
// LOAD FACTOR = size / bucketCount.  When it passes 0.75 the chains are getting
// long, so we REHASH: make twice as many buckets and re-insert every node.
// Rehashing costs O(n) but happens rarely, so put() is still amortised O(1).
//
// RULE OF THREE: we own raw nodes, so we write a destructor that frees every
// chain. Copying is disabled (= delete) so two maps can never free the same nodes.
template <class K, class V>
class HashMap {
private:
    struct Node {
        K key;
        V value;
        Node* next;                              // next node in the same bucket
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Node** buckets;   // array of pointers; buckets[i] is the head of chain i
    int numBuckets;   // length of that array
    int numItems;     // how many key/value pairs are stored

    int indexFor(const K& key, int bucketCountToUse) const {
        return static_cast<int>(hashKey(key) % static_cast<unsigned long>(bucketCountToUse));
    }

    // Double the number of buckets and move every node into its new bucket.
    void rehash() {
        int newCount = numBuckets * 2;
        Node** newBuckets = new Node*[newCount];
        for (int i = 0; i < newCount; ++i) newBuckets[i] = nullptr;

        for (int i = 0; i < numBuckets; ++i) {
            Node* cur = buckets[i];
            while (cur != nullptr) {
                Node* nextNode = cur->next;      // remember before we relink
                int idx = indexFor(cur->key, newCount);
                cur->next = newBuckets[idx];     // push node on the new chain
                newBuckets[idx] = cur;
                cur = nextNode;
            }
        }
        delete[] buckets;
        buckets = newBuckets;
        numBuckets = newCount;
    }

public:
    HashMap() : buckets(nullptr), numBuckets(16), numItems(0) {
        buckets = new Node*[numBuckets];
        for (int i = 0; i < numBuckets; ++i) buckets[i] = nullptr;
    }

    // Copying disabled: see RULE OF THREE note above.
    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

    // Destructor: walk every chain and delete every node, then the bucket array.
    ~HashMap() {
        for (int i = 0; i < numBuckets; ++i) {
            Node* cur = buckets[i];
            while (cur != nullptr) {
                Node* nextNode = cur->next;
                delete cur;
                cur = nextNode;
            }
        }
        delete[] buckets;
    }

    // Insert a new key, or overwrite the value if the key already exists.
    // Average O(1).
    void put(const K& key, const V& value) {
        int idx = indexFor(key, numBuckets);
        for (Node* cur = buckets[idx]; cur != nullptr; cur = cur->next) {
            if (cur->key == key) { cur->value = value; return; }   // overwrite
        }
        Node* node = new Node(key, value);
        node->next = buckets[idx];               // insert at the head of the chain
        buckets[idx] = node;
        ++numItems;
        if (loadFactor() > 0.75) rehash();
    }

    // Returns a pointer to the stored value, or nullptr if the key is missing.
    // Average O(1).
    V* find(const K& key) {
        int idx = indexFor(key, numBuckets);
        for (Node* cur = buckets[idx]; cur != nullptr; cur = cur->next) {
            if (cur->key == key) return &cur->value;
        }
        return nullptr;
    }

    const V* find(const K& key) const {
        int idx = indexFor(key, numBuckets);
        for (const Node* cur = buckets[idx]; cur != nullptr; cur = cur->next) {
            if (cur->key == key) return &cur->value;
        }
        return nullptr;
    }

    bool contains(const K& key) const { return find(key) != nullptr; }

    // Remove a key. Returns false if it was not there. Average O(1).
    bool remove(const K& key) {
        int idx = indexFor(key, numBuckets);
        Node* prev = nullptr;
        for (Node* cur = buckets[idx]; cur != nullptr; prev = cur, cur = cur->next) {
            if (cur->key == key) {
                if (prev == nullptr) buckets[idx] = cur->next;   // removing the head
                else                 prev->next = cur->next;     // bypass the node
                delete cur;
                --numItems;
                return true;
            }
        }
        return false;
    }

    int    size() const        { return numItems; }
    int    bucketCount() const { return numBuckets; }
    double loadFactor() const  { return static_cast<double>(numItems) / numBuckets; }

    // Visit every pair. F is any callable: f(key, value). O(buckets + n).
    // (Order is by bucket, so it is NOT sorted.)
    template <class F>
    void forEach(F f) const {
        for (int i = 0; i < numBuckets; ++i) {
            for (const Node* cur = buckets[i]; cur != nullptr; cur = cur->next) {
                f(cur->key, cur->value);
            }
        }
    }
};
