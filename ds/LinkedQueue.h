#pragma once

// ============================================================================
// LinkedQueue<T>  -  our own FIFO queue (replaces std::queue)
// ============================================================================
// FIFO = First In, First Out, like a line at a ticket counter.
//
//   frontNode                       rearNode
//      |                               |
//      v                               v
//   [ A ] -> [ B ] -> [ C ] -> nullptr          dequeue removes A, enqueue adds after C
//
// We keep BOTH a front and a rear pointer, so enqueue (at rear) and dequeue
// (at front) are O(1). With only a front pointer, enqueue would have to walk
// the whole list: O(n).
//
// RULE OF THREE: the queue owns heap nodes, so the destructor frees them.
// Copy constructor and copy assignment are deleted: a copy would share the same
// nodes and both queues would try to delete them (double free).
template <class T>
class LinkedQueue {
private:
    struct Node {
        T     data;
        Node* next;
        explicit Node(const T& d) : data(d), next(nullptr) {}
    };

    Node* frontNode;   // oldest item (next to leave)
    Node* rearNode;    // newest item (last to leave)
    int   count;

public:
    LinkedQueue() : frontNode(nullptr), rearNode(nullptr), count(0) {}

    LinkedQueue(const LinkedQueue&) = delete;
    LinkedQueue& operator=(const LinkedQueue&) = delete;

    // Destructor: delete every node still waiting in the queue.
    ~LinkedQueue() {
        while (frontNode != nullptr) {
            Node* old = frontNode;
            frontNode = frontNode->next;
            delete old;
        }
    }

    // Add to the back.  O(1)
    void enqueue(const T& item) {
        Node* node = new Node(item);
        if (rearNode == nullptr) {            // queue was empty
            frontNode = rearNode = node;
        } else {
            rearNode->next = node;            // link after the current last node
            rearNode = node;
        }
        ++count;
    }

    // Remove from the front and give the value back through 'out'.  O(1)
    // Returns false (and touches nothing) when the queue is empty.
    bool dequeue(T& out) {
        if (frontNode == nullptr) return false;     // never dereference nullptr
        Node* old = frontNode;
        out = old->data;
        frontNode = frontNode->next;
        if (frontNode == nullptr) rearNode = nullptr;   // queue became empty
        delete old;
        --count;
        return true;
    }

    // Look at the front item without removing it.  O(1)
    bool peek(T& out) const {
        if (frontNode == nullptr) return false;
        out = frontNode->data;
        return true;
    }

    bool isEmpty() const { return frontNode == nullptr; }
    int  size() const    { return count; }
};
