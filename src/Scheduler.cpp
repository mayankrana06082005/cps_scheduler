#include "Scheduler.h"
#include "Comparators.h"
#include <iostream>
#include <iomanip>
#include <cmath>
#include <algorithm>
#include <fstream>

Scheduler::Scheduler(int nodes, double m_delay, double s_delay, int ram_per_node) 
    : current_time(0), num_nodes(nodes), mean_delay(m_delay), stddev_delay(s_delay), total_misses(0), total_completed(0) {
    
    // Seed the random number generator for realistic network jitter
    std::random_device rd;
    rng = std::mt19937(rd());
    
    running_jobs.resize(num_nodes, nullptr);
    ready_queues.resize(num_nodes);
    
    // Initialize RAM capacity and usage trackers for all nodes
    node_memory_capacity.resize(num_nodes, ram_per_node);
    node_memory_used.resize(num_nodes, 0);
}

int Scheduler::get_dynamic_delay() {
    std::normal_distribution<double> dist(mean_delay, stddev_delay);
    int delay = std::round(dist(rng));
    return std::max(1, delay); // Physical packets cannot travel back in time
}

void Scheduler::add_task(const Task& task) { task_set.push_back(task); }
void Scheduler::clear_tasks() { task_set.clear(); }

Task Scheduler::get_task_by_id(int id) const {
    for (const auto& t : task_set) { if (t.id == id) return t; }
    return task_set[0]; 
}

void Scheduler::check_schedulability() const {
    std::vector<float> node_utilization(num_nodes, 0.0f);
    for (const auto& task : task_set) {
        node_utilization[task.node_id] += (float)task.wcet / task.period;
    }
    
    std::cout << "--- Distributed Schedulability Analysis ---\n";
    for (int i = 0; i < num_nodes; i++) {
        std::cout << "Node " << i << " Utilization: " << std::fixed << std::setprecision(3) << node_utilization[i] << "\n";
        if(node_utilization[i] > 1.0) {
            std::cout << "  -> WARNING: Node " << i << " is OVERLOADED!\n";
        }
    }
    std::cout << "-------------------------------------------\n";
}

void Scheduler::generate_jobs(int duration) {
    int job_id_counter = 1;
    for (const auto& task : task_set) {
        for (int t = 0; t < duration; t += task.period) {
            JobPtr new_job = std::make_shared<Job>();
            new_job->id = job_id_counter++;
            new_job->task_id = task.id;
            new_job->node_id = task.node_id; 
            new_job->release_time = t;
            new_job->absolute_deadline = t + task.relative_deadline;
            new_job->remaining_time = task.wcet;
            new_job->parent_period = task.period;
            new_job->pending_dependencies = task.num_predecessors;
            new_job->state = JobState::WAITING;
            new_job->last_start_time = 0;
            
            // Pass the RAM constraints down from the Task to the Job
            new_job->memory_required = task.memory_required;
            new_job->memory_allocated = false;
            
            all_jobs.push_back(new_job);
        }
    }
}

void Scheduler::check_network() {
    for (auto it = network_queue.begin(); it != network_queue.end(); ) {
        if (it->arrival_time == current_time) {
            for (auto& job : all_jobs) {
                if (job->task_id == it->dest_task_id && (job->state == JobState::BLOCKED || job->state == JobState::WAITING)) {
                    job->pending_dependencies--;
                    if (job->pending_dependencies == 0 && job->state == JobState::BLOCKED) {
                        job->state = JobState::READY;
                        ready_queues[job->node_id].push(job);
                    }
                    break; 
                }
            }
            it = network_queue.erase(it); 
        } else {
            it++;
        }
    }
}

void Scheduler::check_arrivals() {
    for (auto& job : all_jobs) {
        if (job->state == JobState::WAITING && job->release_time == current_time) {
            if (job->pending_dependencies > 0) {
                job->state = JobState::BLOCKED;
            } else {
                job->state = JobState::READY;
                ready_queues[job->node_id].push(job);
            }
        }
    }
}

void Scheduler::check_running_jobs() {
    for (int n = 0; n < num_nodes; n++) {
        if (!running_jobs[n]) continue;

        if (running_jobs[n]->remaining_time == 0) {
            gantt_log.push_back({n, running_jobs[n]->task_id, running_jobs[n]->last_start_time, current_time});
            
            // --- OS MEMORY MANAGEMENT ---
            // The process has finished executing. We must free its memory block.
            node_memory_used[n] -= running_jobs[n]->memory_required;
            
            Task parent_task = get_task_by_id(running_jobs[n]->task_id);
            for (int succ_id : parent_task.successor_ids) {
                NetworkMessage msg;
                msg.source_node = n;
                msg.dest_task_id = succ_id;
                msg.dest_node = get_task_by_id(succ_id).node_id;
                
                int actual_delay = get_dynamic_delay();
                msg.arrival_time = current_time + actual_delay;
                
                network_queue.push_back(msg);
            }

            running_jobs[n]->state = JobState::COMPLETED;
            total_completed++;
            running_jobs[n] = nullptr;
        } 
        else if (current_time > running_jobs[n]->absolute_deadline) {
            std::cout << "[t=" << current_time << "] !!! MISS !!! Task " << running_jobs[n]->task_id 
                      << " missed deadline on Node " << n << "\n";
            running_jobs[n]->state = JobState::MISSED;
            
            // Ensure memory is freed even if the task crashed/missed deadline
            node_memory_used[n] -= running_jobs[n]->memory_required;
            
            total_misses++;
            running_jobs[n] = nullptr;
        }
    }
}

void Scheduler::dispatch(Policy policy) {
    std::function<bool(const JobPtr&, const JobPtr&)> compare;
    if (policy == Policy::RMS) compare = RmsComparator();
    else if (policy == Policy::EDF) compare = EdfComparator();
    else compare = LlfComparator();

    for (int n = 0; n < num_nodes; n++) {
        if (ready_queues[n].empty()) continue;

        JobPtr top_job = ready_queues[n].top();

        // --- OS MEMORY MANAGEMENT ---
        // Before a task can be scheduled onto the CPU, it must allocate RAM.
        // If it is preempted later, it keeps its RAM. It only allocations once.
        if (!top_job->memory_allocated) {
            if (node_memory_used[n] + top_job->memory_required <= node_memory_capacity[n]) {
                node_memory_used[n] += top_job->memory_required;
                top_job->memory_allocated = true;
            } else {
                // MEMORY BOTTLENECK!
                // The task has the highest priority, but there is no RAM available.
                // It remains blocked in the ready queue, and the CPU sits idle 
                // until another task finishes and frees up memory.
                continue; 
            }
        }

        if (!running_jobs[n]) {
            running_jobs[n] = top_job;
            ready_queues[n].pop();
            running_jobs[n]->state = JobState::RUNNING;
            running_jobs[n]->last_start_time = current_time;
        } 
        else {
            if (compare(running_jobs[n], top_job)) {
                gantt_log.push_back({n, running_jobs[n]->task_id, running_jobs[n]->last_start_time, current_time});
                running_jobs[n]->state = JobState::READY;
                ready_queues[n].push(running_jobs[n]);
                running_jobs[n] = top_job;
                ready_queues[n].pop();
                running_jobs[n]->state = JobState::RUNNING;
                running_jobs[n]->last_start_time = current_time;
            }
        }
    }
}

void Scheduler::run(int duration, Policy policy) {
    std::string pol_name = (policy == Policy::RMS) ? "RMS" : (policy == Policy::EDF) ? "EDF" : "LLF";
    std::cout << "\n=== Starting Distributed Simulation: " << pol_name << " ===\n";
    std::cout << "Nodes: " << num_nodes << " | Mean Delay: " << mean_delay << "ms | StdDev: " << stddev_delay << "ms\n";
    std::cout << "Simulating with Gaussian Network Noise & Memory Bottlenecks...\n";
    
    current_time = 0;
    total_misses = 0;
    total_completed = 0;
    all_jobs.clear();
    network_queue.clear();
    gantt_log.clear(); 
    
    // Reset RAM usage for fresh run
    std::fill(node_memory_used.begin(), node_memory_used.end(), 0);
    
    for(int n=0; n<num_nodes; n++) {
        running_jobs[n] = nullptr;
        if (policy == Policy::RMS) ready_queues[n] = JobQueue(RmsComparator());
        else if (policy == Policy::EDF) ready_queues[n] = JobQueue(EdfComparator());
        else ready_queues[n] = JobQueue(LlfComparator());
    }

    generate_jobs(duration);

    while (current_time < duration) {
        check_network();       
        check_running_jobs();  
        check_arrivals();      
        dispatch(policy);      

        for (int n = 0; n < num_nodes; n++) {
            if (running_jobs[n]) running_jobs[n]->remaining_time--;
        }

        current_time++; 
    }
    std::cout << "=== Simulation Ended ===\n";
}

void Scheduler::print_metrics() const {
    int total_jobs = total_completed + total_misses;
    float miss_rate = total_jobs == 0 ? 0 : (float)total_misses / total_jobs * 100.0f;
    std::cout << "Completed:  " << total_completed << " | Missed: " << total_misses << "\n";
    std::cout << "Miss Rate:  " << std::fixed << std::setprecision(2) << miss_rate << "%\n";
}

void Scheduler::export_gantt_csv(const std::string& filename) const {
    std::ofstream file(filename);
    file << "Node,Task,Start,End\n";
    for (const auto& record : gantt_log) {
        file << record.node_id << "," << record.task_id << ","
             << record.start_time << "," << record.end_time << "\n";
    }
    std::cout << "Detailed Trace Log saved to: " << filename << "\n";
}