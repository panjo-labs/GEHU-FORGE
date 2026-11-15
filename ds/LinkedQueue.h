#pragma once

// LinkedQueue<T>: apni FIFO queue (std::queue ki jagah).
// front + rear dono pointer rakhe, isliye enqueue/dequeue dono O(1).
// Nodes hamare hain: destructor free karta hai, copy delete ki (warna double free).
template <class T>
class LinkedQueue {
private:
    struct Node {
        T     data;
        Node* next;
        explicit Node(const T& d) : data(d), next(nullptr) {}
    };

    Node* frontNode;   // sabse purana (pehle niklega)
    Node* rearNode;    // sabse naya
    int   count;

public:
    LinkedQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}

    LinkedQueue(const LinkedQueue&) = delete;
    LinkedQueue& operator=(const LinkedQueue&) = delete;

    ~LinkedQueue() {
        while (frontNode != nullptr) {
            Node* old = frontNode;
            frontNode = frontNode->next;
            delete old;
        }
    }

    // Peeche add.  O(1)
    void enqueue(const T& item) {
        Node* node = new Node(item);
        if (rearNode == nullptr) {
            frontNode = rearNode = node;
        } else {
            rearNode->next = node;
            rearNode = node;
        }
        ++count;
    }

    // Aage se nikaalo, value 'out' mein. Khaali queue par false.  O(1)
    bool dequeue(T& out) {
        if (frontNode == nullptr) return false;
        Node* old = frontNode;
        out = old->data;
        frontNode = frontNode->next;
        if (frontNode == nullptr) rearNode = nullptr;   // queue khaali ho gayi
        delete old;
        --count;
        return true;
    }

    // Front dekho, hatao mat.  O(1)
    bool peek(T& out) const {
        if (frontNode == nullptr) return false;
        out = frontNode->data;
        return true;
    }

    bool isEmpty() const { return frontNode == nullptr; }
    int  size() const    { return count; }
};
