#pragma once

#include "Achievement.h"
#include "ds/LinkedQueue.h"
#include "ds/HashMap.h"

// VerificationService: naye submissions ki FIFO queue. Sirf review ka order + decision,
// points RankEngine deta hai. Pointer nahi, id rakhte hain - id kabhi dangle nahi hoti.
class VerificationService {
private:
    LinkedQueue<int> pending;   // review ke liye ids, sabse purana front par

public:
    // Pending achievement ko queue ke peeche daalo. O(1)
    bool submit(Achievement* a) {
        if (a == nullptr) return false;
        if (a->getStatus() != VerificationStatus::Pending) return false;
        pending.enqueue(a->getId());
        return true;
    }

    // Sabse purani id nikaalo, registry se object dhundo (O(1)), approve/reject karo.
    // Queue khaali ya id na mile toh nullptr.
    Achievement* reviewNext(const HashMap<int, Achievement*>& registry, bool approve) {
        int id = 0;
        if (!pending.dequeue(id)) return nullptr;
        Achievement* const* found = registry.find(id);
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
