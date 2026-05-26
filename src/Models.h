#pragma once
#include <string>
#include <memory>

// G1: Task Type definition
enum class TaskType { SENSOR, CONTROL, ACTUATOR };

// G2: Job Lifecycle States
// Notice the BLOCKED state is already here in preparation for Tier 4 (Distributed computing)
enum class JobState { WAITING, READY, RUNNING, BLOCKED, COMPLETED, MISSED };

// G1: The Task Model
// A Task is a TEMPLATE. It doesn't execute; it generates Jobs.
struct Task {
    int id;
    int period;
    int wcet;              // Worst-Case Execution Time
    int relative_deadline;
    TaskType type;
};

// G2: The Job Lifecycle
// A Job is a specific INSTANCE of a Task that runs in a specific time window.
struct Job {
    int id;                 // Unique ID for this specific job
    int task_id;            // Which task generated this job?
    int release_time;       // When does this job become READY?
    int absolute_deadline;  // release_time + task.relative_deadline
    int remaining_time;     // Starts at WCET, counts down to 0
    JobState state;
    
    // We store the parent's period here so RMS can quickly access it 
    // without needing to look up the original Task object every time.
    int parent_period;      
};

// We use shared_ptr so we can pass jobs between queues without copying the data
using JobPtr = std::shared_ptr<Job>;