#pragma once // HEADER GUARD: the compiler reads this file only once per program

#include <cstddef>

// ============================================================================
// DynArray<T>  -  our own "growable array" (replaces std::vector)
// ============================================================================
// IDEA: A normal array has a fixed size. DynArray keeps a raw array on the heap
// and, when it becomes full, allocates an array TWICE as big and copies the
// old items across. Doubling makes push_back "amortised O(1)":
// a copy of n items happens only after n cheap pushes, so the average cost per
// push stays constant.
//
// OOP TOPICS USED HERE:
//   * Templates       : one class works for int, double, std::string, pointers...
//   * Rule of Three   : we own heap memory, so we must write the destructor,
//                       the copy constructor and the copy assignment ourselves.
//   * Deep copy       : copies the ELEMENTS, not just the pointer (otherwise two
//                       objects would share one array and delete it twice).
template <class T>
class DynArray {
private:
    T*  data;      // pointer to the heap array
    int count;     // how many items are stored right now
    int cap;       // how many items fit before we must grow

    // Allocate a bigger array, copy everything, free the old one.  O(n)
    void grow() {
        int newCap = (cap == 0) ? 4 : cap * 2;      // start at 4, then double
        T* bigger = new T[newCap];
        for (int i = 0; i < count; ++i) bigger[i] = data[i];
        delete[] data;                              // give old memory back
        data = bigger;
        cap  = newCap;
    }

public:
    // Constructor: starts empty with no memory allocated.
    DynArray() : data(nullptr), count(0), cap(0) {}

    // Copy constructor (Rule of Three #2): builds an independent DEEP copy.
    DynArray(const DynArray& other) : data(nullptr), count(0), cap(0) {
        for (int i = 0; i < other.count; ++i) push_back(other.data[i]);
    }

    // Copy assignment (Rule of Three #3): a = b;
    DynArray& operator=(const DynArray& other) {
        if (this == &other) return *this;           // protect against a = a;
        clear();
        for (int i = 0; i < other.count; ++i) push_back(other.data[i]);
        return *this;
    }

    // Destructor (Rule of Three #1): frees the heap array automatically.
    ~DynArray() { delete[] data; }

    // Add an item at the end.  Amortised O(1).
    void push_back(const T& item) {
        if (count == cap) grow();
        data[count++] = item;
    }

    // Index access.  O(1).  (No bounds check: callers must stay in 0..size()-1)
    T&       operator[](int i)       { return data[i]; }
    const T& operator[](int i) const { return data[i]; }

    int  size() const     { return count; }
    int  capacity() const { return cap; }
    bool empty() const    { return count == 0; }

    // Forget all items but keep the memory for reuse.  O(1) for simple types.
    void clear() { count = 0; }

    // Remove the item at position i and shift the rest left.  O(n).
    bool removeAt(int i) {
        if (i < 0 || i >= count) return false;
        for (int j = i; j + 1 < count; ++j) data[j] = data[j + 1];
        --count;
        return true;
    }
};
