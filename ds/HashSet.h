#pragma once

#include "HashMap.h"

// HashSet<T>: duplicate-free collection (std::unordered_set ki jagah).
// Andar HashMap<T,bool> hai - hashing/chaining/rehash wahi se reuse. add/contains/remove avg O(1).
template <class T>
class HashSet {
private:
    HashMap<T, bool> map;   // value hamesha true, use nahi hoti

public:
    // Naya item ho toh true, pehle se ho toh false.
    bool add(const T& item) {
        if (map.contains(item)) return false;
        map.put(item, true);
        return true;
    }

    bool contains(const T& item) const { return map.contains(item); }
    bool remove(const T& item)         { return map.remove(item); }
    int  size() const                  { return map.size(); }

    template <class F>
    void forEach(F f) const {
        map.forEach([&f](const T& key, const bool&) { f(key); });
    }
};
