#pragma once
#include "DynArray.h"

// Binary min-heap (array mein). Top-K ke liye: size K rakho, chhota wala bahar.
// push/pop O(log K), top O(1).
// Less(a, b) true matlab a "chhota" hai, yaani pehle bahar niklega.

template <class T, class Less>
class MinHeap {
private:
    DynArray<T> a;
    Less less;

    void swapAt(int i, int j) {
        T t = a[i];
        a[i] = a[j];
        a[j] = t;
    }

    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (!less(a[i], a[p])) break;
            swapAt(i, p);
            i = p;
        }
    }

    void siftDown(int i) {
        int n = a.size();
        while (true) {
            int l = 2 * i + 1, r = l + 1, m = i;
            if (l < n && less(a[l], a[m])) m = l;
            if (r < n && less(a[r], a[m])) m = r;
            if (m == i) break;
            swapAt(i, m);
            i = m;
        }
    }

public:
    int size() const { return a.size(); }
    bool empty() const { return a.size() == 0; }
    const T& top() const { return a[0]; }

    void push(const T& x) {
        a.push_back(x);
        siftUp(a.size() - 1);
    }

    T pop() {
        T root = a[0];
        int last = a.size() - 1;
        a[0] = a[last];
        a.removeAt(last);
        if (!empty()) siftDown(0);
        return root;
    }
};
