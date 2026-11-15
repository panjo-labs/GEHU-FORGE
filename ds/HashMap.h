#pragma once

#include <string>

// HashMap<K,V>: separate chaining wala apna hash table (std::unordered_map ki jagah).
// put/find/remove avg O(1), worst O(n) jab sab ek hi bucket mein gire.

// int key ke liye number khud hi hash hai.
inline unsigned long hashKey(int key) {
    return static_cast<unsigned long>(key < 0 ? -key : key);
}

// String ke liye polynomial rolling hash: h = h*31 + char ("abc" != "acb").
inline unsigned long hashKey(const std::string& key) {
    unsigned long h = 0;
    for (size_t i = 0; i < key.size(); ++i) {
        h = h * 31 + static_cast<unsigned char>(key[i]);
    }
    return h;
}

// Load factor 0.75 se upar gaya toh rehash: buckets double, sab node dobara daalo.
// Rehash O(n) hai par kabhi-kabhi hota hai, isliye put() amortised O(1).
// Rule of Three: nodes hamare hain - destructor free karta hai, copy delete ki hai.
template <class K, class V>
class HashMap {
private:
    struct Node {
        K key;
        V value;
        Node* next;                              // same bucket ka agla node
        Node(const K& k, const V& v) : key(k), value(v), next(nullptr) {}
    };

    Node** buckets;   // buckets[i] = chain i ka head
    int numBuckets;
    int numItems;

    int indexFor(const K& key, int bucketCountToUse) const {
        return static_cast<int>(hashKey(key) % static_cast<unsigned long>(bucketCountToUse));
    }

    // Buckets double karo, har node ko naye bucket mein shift karo.  O(n)
    void rehash() {
        int newCount = numBuckets * 2;
        Node** newBuckets = new Node*[newCount];
        for (int i = 0; i < newCount; ++i) newBuckets[i] = nullptr;

        for (int i = 0; i < numBuckets; ++i) {
            Node* cur = buckets[i];
            while (cur != nullptr) {
                Node* nextNode = cur->next;      // relink se pehle yaad rakho
                int idx = indexFor(cur->key, newCount);
                cur->next = newBuckets[idx];
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

    HashMap(const HashMap&) = delete;
    HashMap& operator=(const HashMap&) = delete;

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

    // Naya key daalo ya purani value overwrite. Avg O(1).
    void put(const K& key, const V& value) {
        int idx = indexFor(key, numBuckets);
        for (Node* cur = buckets[idx]; cur != nullptr; cur = cur->next) {
            if (cur->key == key) { cur->value = value; return; }
        }
        Node* node = new Node(key, value);
        node->next = buckets[idx];               // chain ke head par daalo
        buckets[idx] = node;
        ++numItems;
        if (loadFactor() > 0.75) rehash();
    }

    // Value ka pointer, key na ho toh nullptr. Avg O(1).
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

    // Key hatao; na mile toh false. Avg O(1).
    bool remove(const K& key) {
        int idx = indexFor(key, numBuckets);
        Node* prev = nullptr;
        for (Node* cur = buckets[idx]; cur != nullptr; prev = cur, cur = cur->next) {
            if (cur->key == key) {
                if (prev == nullptr) buckets[idx] = cur->next;
                else                 prev->next = cur->next;
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

    // Har pair par f(key, value). O(buckets + n), order sorted nahi hai.
    template <class F>
    void forEach(F f) const {
        for (int i = 0; i < numBuckets; ++i) {
            for (const Node* cur = buckets[i]; cur != nullptr; cur = cur->next) {
                f(cur->key, cur->value);
            }
        }
    }
};
