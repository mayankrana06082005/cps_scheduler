#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include "Models.h"

// Define the policies we support
enum class Policy { RMS, EDF, LLF };

// This alias defines the complex type signature for our dynamic priority queue (G3)
using JobQueue = std::priority_queue<JobPtr, std::vector<JobPtr>, std::function<bool(const JobPtr&, const JobPtr&)>>;

class Scheduler {
private:
    int current_time;
    std::vector<Task> task_set;
    std::vector<JobPtr> all_jobs; // Holds all generated jobs in WAITING state
    
    JobPtr running_job;
    JobQueue ready_queue; // The G3 Ready Queue
    
    int total_misses;
    int total_completed;

    // Helper functions for the tick loop
    void generate_jobs(int simulation_duration);
    void check_arrivals();
    void check_running_job();
    void dispatch(Policy policy);

public:
    Scheduler();
    void add_task(const Task& task);
    
    // The main simulation loop
    void run(int duration, Policy policy);
    
    void print_metrics() const;
};