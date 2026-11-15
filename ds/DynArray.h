#pragma once

#include <cstddef>

// DynArray<T>: apna growable array (std::vector ki jagah).
// Full hone par capacity double - isliye push_back amortised O(1). Index O(1).
template <class T>
class DynArray {
private:
    T*  data;
    int count;
    int cap;

    // Double size ka naya array, sab copy, purana free.  O(n)
    void grow() {
        int newCap = (cap == 0) ? 4 : cap * 2;
        T* bigger = new T[newCap];
        for (int i = 0; i < count; ++i) bigger[i] = data[i];
        delete[] data;
        data = bigger;
        cap  = newCap;
    }

public:
    DynArray() : data(nullptr), count(0), cap(0) {}

    // Rule of Three: heap memory hamari hai, isliye copy ctor + assignment + destructor khud likhe.
    // Deep copy - warna do objects ek hi array share karke double delete karenge.
    DynArray(const DynArray& other) : data(nullptr), count(0), cap(0) {
        for (int i = 0; i < other.count; ++i) push_back(other.data[i]);
    }

    DynArray& operator=(const DynArray& other) {
        if (this == &other) return *this;           // a = a se bachao
        clear();
        for (int i = 0; i < other.count; ++i) push_back(other.data[i]);
        return *this;
    }

    ~DynArray() { delete[] data; }

    // Amortised O(1)
    void push_back(const T& item) {
        if (count == cap) grow();
        data[count++] = item;
    }

    // O(1), bounds check nahi hai - 0..size()-1 ke andar hi raho.
    T&       operator[](int i)       { return data[i]; }
    const T& operator[](int i) const { return data[i]; }

    int  size() const     { return count; }
    int  capacity() const { return cap; }
    bool empty() const    { return count == 0; }

    // Items bhool jao, memory reuse ke liye rakho.
    void clear() { count = 0; }

    // Position i hatao, baaki left shift.  O(n)
    bool removeAt(int i) {
        if (i < 0 || i >= count) return false;
        for (int j = i; j + 1 < count; ++j) data[j] = data[j + 1];
        --count;
        return true;
    }
};
