#include "Scheduler.h"
#include <iostream>

int main() {
    Scheduler scheduler;

    // A classic Liu & Layland test case.
    // Task format: ID, Period, WCET, Relative Deadline, Type
    scheduler.add_task({1, 4, 1, 4, TaskType::SENSOR});
    scheduler.add_task({2, 5, 2, 5, TaskType::CONTROL});
    scheduler.add_task({3, 10, 2, 10, TaskType::ACTUATOR});

    // Utilization U = (1/4) + (2/5) + (2/10) = 0.85
    // RMS bound for 3 tasks is 0.78. Since 0.85 > 0.78, RMS will likely fail.

    int sim_time = 20; // Simulate 20 virtual milliseconds

    // Run with Rate Monotonic Scheduling (RMS)
    scheduler.run(sim_time, Policy::RMS);
    scheduler.print_metrics();

    std::cout << "\n----------------------------------------\n";

    // Run the exact same tasks with Earliest Deadline First (EDF)
    scheduler.run(sim_time, Policy::EDF);
    scheduler.print_metrics();

    return 0;
}