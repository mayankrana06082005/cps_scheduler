#include "Scheduler.h"
#include "Comparators.h"
#include <iostream>
#include <iomanip>

Scheduler::Scheduler() : current_time(0), running_job(nullptr), total_misses(0), total_completed(0) {
    // Initialize ready_queue with a dummy comparator to prevent crashes
    // It gets overwritten in run()
    ready_queue = JobQueue(RmsComparator());
}

void Scheduler::add_task(const Task& task) {
    task_set.push_back(task);
}

// Unrolls Tasks into individual Jobs based on their periods
void Scheduler::generate_jobs(int duration) {
    int job_id_counter = 1;
    for (const auto& task : task_set) {
        for (int t = 0; t < duration; t += task.period) {
            JobPtr new_job = std::make_shared<Job>();
            new_job->id = job_id_counter++;
            new_job->task_id = task.id;
            new_job->release_time = t;
            new_job->absolute_deadline = t + task.relative_deadline;
            new_job->remaining_time = task.wcet;
            new_job->state = JobState::WAITING;
            new_job->parent_period = task.period;
            
            all_jobs.push_back(new_job);
        }
    }
}

// 1. Move jobs from WAITING to READY when their release time hits
void Scheduler::check_arrivals() {
    for (auto& job : all_jobs) {
        if (job->state == JobState::WAITING && job->release_time == current_time) {
            job->state = JobState::READY;
            ready_queue.push(job);
            std::cout << "[t=" << current_time << "] ARRIVAL: Task " << job->task_id 
                      << " Job " << job->id << " (Deadline: " << job->absolute_deadline << ")\n";
        }
    }
}

// 2. Check if the currently running job has finished or missed its deadline
void Scheduler::check_running_job() {
    if (!running_job) return;

    if (running_job->remaining_time == 0) {
        std::cout << "[t=" << current_time << "] COMPLETED: Task " << running_job->task_id << "\n";
        running_job->state = JobState::COMPLETED;
        total_completed++;
        running_job = nullptr;
    } 
    else if (current_time > running_job->absolute_deadline) {
        std::cout << "[t=" << current_time << "] MISS! Task " << running_job->task_id 
                  << " missed deadline of " << running_job->absolute_deadline << "\n";
        running_job->state = JobState::MISSED;
        total_misses++;
        running_job = nullptr; // Terminate the missed job
    }
}

// 3. The Dispatcher (Preemption Logic - G6)
void Scheduler::dispatch(Policy policy) {
    if (ready_queue.empty()) return;

    // We need to inject the correct comparator function dynamically
    std::function<bool(const JobPtr&, const JobPtr&)> compare;
    if (policy == Policy::RMS) compare = RmsComparator();
    else if (policy == Policy::EDF) compare = EdfComparator();
    else compare = LlfComparator();

    JobPtr top_job = ready_queue.top();

    if (!running_job) {
        // CPU is idle, just pop and run
        running_job = top_job;
        ready_queue.pop();
        running_job->state = JobState::RUNNING;
        std::cout << "[t=" << current_time << "] DISPATCH: Task " << running_job->task_id << "\n";
    } 
    else {
        // Preemption Check: Is the top of the queue higher priority than the running job?
        if (compare(running_job, top_job)) {
            std::cout << "[t=" << current_time << "] PREEMPTION: Task " << top_job->task_id 
                      << " preempts Task " << running_job->task_id << "\n";
            
            // Context Switch
            running_job->state = JobState::READY;
            ready_queue.push(running_job);
            
            running_job = top_job;
            ready_queue.pop();
            running_job->state = JobState::RUNNING;
        }
    }
}

// The main Simulator Time Loop
void Scheduler::run(int duration, Policy policy) {
    std::string pol_name = (policy == Policy::RMS) ? "RMS" : (policy == Policy::EDF) ? "EDF" : "LLF";
    std::cout << "\n=== Starting Simulation: " << pol_name << " ===\n";
    
    current_time = 0;
    total_misses = 0;
    total_completed = 0;
    all_jobs.clear();
    running_job = nullptr;
    
    // Inject the correct sorting policy into our ready queue
    if (policy == Policy::RMS) ready_queue = JobQueue(RmsComparator());
    else if (policy == Policy::EDF) ready_queue = JobQueue(EdfComparator());
    else ready_queue = JobQueue(LlfComparator());

    generate_jobs(duration);

    // The Virtual Clock
    while (current_time < duration) {
        check_running_job();
        check_arrivals();
        dispatch(policy);

        // 4. Execution Phase
        if (running_job) {
            running_job->remaining_time--;
        }

        current_time++; // Advance virtual time
    }
    
    std::cout << "=== Simulation Ended at t=" << duration << " ===\n";
}

// G7: Basic Metrics Output
void Scheduler::print_metrics() const {
    int total_jobs = total_completed + total_misses;
    float miss_rate = total_jobs == 0 ? 0 : (float)total_misses / total_jobs * 100.0f;
    std::cout << "Total Jobs: " << total_jobs << "\n";
    std::cout << "Completed:  " << total_completed << "\n";
    std::cout << "Missed:     " << total_misses << "\n";
    std::cout << "Miss Rate:  " << std::fixed << std::setprecision(2) << miss_rate << "%\n";
}