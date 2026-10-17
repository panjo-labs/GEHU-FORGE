#pragma once

#include "HashMap.h"

// ============================================================================
// HashSet<T>  -  a collection with NO duplicates (replaces std::unordered_set)
// ============================================================================
// DESIGN CHOICE: COMPOSITION ("has-a"). A set is just a map where we only care
// about the keys. So HashSet HAS-A HashMap<T,bool> and reuses its hashing,
// chaining and rehashing instead of repeating the code.
//
// Used by Student to store skills: adding "C++" twice keeps only one "C++".
// add / contains / remove are average O(1).
template <class T>
class HashSet {
private:
    HashMap<T, bool> map;   // the value is always 'true' and is ignored

public:
    // Returns true if the item was new, false if it was already present.
    bool add(const T& item) {
        if (map.contains(item)) return false;
        map.put(item, true);
        return true;
    }

    bool contains(const T& item) const { return map.contains(item); }
    bool remove(const T& item)         { return map.remove(item); }
    int  size() const                  { return map.size(); }

    // Visit every item: f(item).
    template <class F>
    void forEach(F f) const {
        map.forEach([&f](const T& key, const bool&) { f(key); });
    }
};
