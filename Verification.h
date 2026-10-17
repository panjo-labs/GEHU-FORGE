#pragma once

#include "Achievement.h"
#include "ds/LinkedQueue.h"
#include "ds/HashMap.h"

// ============================================================================
// VerificationService  -  the "staging area" for new achievements
// ============================================================================
// FLOW:   student submits -> submit() puts the id in a FIFO queue
//         reviewer calls reviewNext() -> oldest submission is approved/rejected
//
// SEPARATION OF CONCERNS: this class only decides WHO is reviewed next and
// records the decision. It does NOT calculate points (RankEngine does that).
//
// WHY STORE IDs AND NOT POINTERS? An id is just a number: it can never dangle.
// The real object is found through the registry (a HashMap, O(1) lookup).
//
// FIFO only decides the ORDER of review. It does not mean "first = approved".
class VerificationService {
private:
    LinkedQueue<int> pending;   // achievement ids waiting for review, oldest at front

public:
    // Add a Pending achievement to the back of the queue.  O(1)
    // Returns false if the achievement is null or already reviewed.
    bool submit(Achievement* a) {
        if (a == nullptr) return false;
        if (a->getStatus() != VerificationStatus::Pending) return false;
        pending.enqueue(a->getId());
        return true;
    }

    // Take the OLDEST id, look the object up, approve or reject it.
    // Returns the reviewed object, or nullptr if the queue is empty / id unknown.
    Achievement* reviewNext(const HashMap<int, Achievement*>& registry, bool approve) {
        int id = 0;
        if (!pending.dequeue(id)) return nullptr;          // empty queue
        Achievement* const* found = registry.find(id);     // O(1)
        if (found == nullptr || *found == nullptr) return nullptr;
        Achievement* a = *found;
        if (approve) a->approve();
        else         a->reject();
        return a;
    }

    bool peekNextId(int& id) const { return pending.peek(id); }
    int  pendingCount() const { return pending.size(); }
    bool hasPending() const { return !pending.isEmpty(); }
};
