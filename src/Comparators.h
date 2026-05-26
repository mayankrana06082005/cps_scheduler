#pragma once
#include "Models.h"

/*
    THE ALGORITHM BRAIN
    std::priority_queue in C++ outputs the "largest" element first.
    Because we want the "smallest" value (e.g., shortest period) to be the highest priority,
    our comparators must return TRUE if left > right.
*/

// G4: Rate Monotonic Scheduling (RMS)
// Static priority based entirely on the task's period.
struct RmsComparator {
    bool operator()(const JobPtr& a, const JobPtr& b) const {
        // Shorter period = higher priority.
        if (a->parent_period == b->parent_period) {
            return a->id > b->id; // Tie-breaker: older jobs go first
        }
        return a->parent_period > b->parent_period;
    }
};

// G5: Earliest Deadline First (EDF)
// Dynamic priority based on which job's absolute deadline is closest.
struct EdfComparator {
    bool operator()(const JobPtr& a, const JobPtr& b) const {
        // Earlier absolute deadline = higher priority.
        if (a->absolute_deadline == b->absolute_deadline) {
            return a->id > b->id; // Tie-breaker
        }
        return a->absolute_deadline > b->absolute_deadline;
    }
};

// Tier 4 Extension (G11): Least Laxity First (LLF)
// Laxity = Time until deadline - Time needed to finish.
// Notice we don't need the current clock time! 
// (Deadline - CurrentTime) - RemainingTime <=> Deadline - RemainingTime.
struct LlfComparator {
    bool operator()(const JobPtr& a, const JobPtr& b) const {
        int laxity_a = a->absolute_deadline - a->remaining_time;
        int laxity_b = b->absolute_deadline - b->remaining_time;
        
        if (laxity_a == laxity_b) {
            return a->id > b->id; // Tie-breaker
        }
        return laxity_a > laxity_b;
    }
};