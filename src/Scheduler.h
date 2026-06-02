#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <functional>
#include <string>
#include <random> 
#include "Models.h"

enum class Policy { RMS, EDF, LLF };
using JobQueue = std::priority_queue<JobPtr, std::vector<JobPtr>, std::function<bool(const JobPtr&, const JobPtr&)>>;

class Scheduler {
private:
    int current_time;
    int num_nodes;
    
    // Dynamic Network Delay Parameters
    double mean_delay;
    double stddev_delay;
    std::mt19937 rng; 

    // Memory Constraints
    std::vector<int> node_memory_capacity;
    std::vector<int> node_memory_used;

    // --- Core Architecture ---
    std::vector<Task> task_set;
    std::vector<JobPtr> all_jobs; 
    
    std::vector<JobPtr> running_jobs; 
    std::vector<JobQueue> ready_queues; 
    
    std::vector<NetworkMessage> network_queue;
    std::vector<GanttRecord> gantt_log;
    
    int total_misses;
    int total_completed;

    void generate_jobs(int simulation_duration);
    Task get_task_by_id(int id) const;
    int get_dynamic_delay(); 
    
    void check_network();
    void check_arrivals();
    void check_running_jobs();
    void dispatch(Policy policy);

public:
    // Constructor
    Scheduler(int nodes, double mean_delay, double stddev_delay, int ram_per_node);
    
    void add_task(const Task& task);
    void clear_tasks();
    void check_schedulability() const;

    void run(int duration, Policy policy);
    void print_metrics() const;
    void export_gantt_csv(const std::string& filename) const;
};