#include "Scheduler.h"
#include <iostream>

int main() {
    // CONSTRUCTOR: 12 Nodes | Mean Delay: 10.0ms | StdDev: 5.0ms | RAM: 256KB per node
    Scheduler scheduler(12, 10.0, 5.0, 256); 
    int sim_time = 500; 

    std::cout << "\n========================================================\n";


    // Format: {id, node, period, wcet, deadline, type, predecessors, {successors}, memory_kb}
    // GROUP A — Inference Pipeline (Period: 100ms)

    scheduler.add_task({1, 0, 100, 2, 100, TaskType::SENSOR, 0, {6}, 16});   // Sensor: 16KB
    scheduler.add_task({2, 1, 100, 2, 100, TaskType::SENSOR, 0, {6, 7}, 16});
    scheduler.add_task({3, 2, 100, 2, 100, TaskType::SENSOR, 0, {7}, 16});
    scheduler.add_task({4, 3, 100, 2, 100, TaskType::SENSOR, 0, {7, 8}, 16});
    scheduler.add_task({5, 4, 100, 2, 100, TaskType::SENSOR, 0, {8}, 16});

    scheduler.add_task({6, 5, 100, 4, 100, TaskType::CONTROL, 2, {9}, 32});  // Filter: 32KB
    scheduler.add_task({7, 6, 100, 4, 100, TaskType::CONTROL, 3, {10}, 32});
    scheduler.add_task({8, 7, 100, 4, 100, TaskType::CONTROL, 2, {11}, 32});

    scheduler.add_task({9,  5, 100, 6, 100, TaskType::CONTROL, 1, {12}, 64}); // Features: 64KB
    scheduler.add_task({10, 6, 100, 6, 100, TaskType::CONTROL, 1, {13}, 64});
    scheduler.add_task({11, 7, 100, 6, 100, TaskType::CONTROL, 1, {14}, 64});

    scheduler.add_task({12, 5, 100, 10, 100, TaskType::CONTROL, 1, {15}, 128}); // TinyML: 128KB
    scheduler.add_task({13, 6, 100, 10, 100, TaskType::CONTROL, 1, {15, 16}, 128}); 
    scheduler.add_task({14, 7, 100, 10, 100, TaskType::CONTROL, 1, {16}, 128});

    scheduler.add_task({15, 8, 100, 3, 100, TaskType::CONTROL, 2, {17}, 32}); // Encrypt: 32KB
    scheduler.add_task({16, 9, 100, 3, 100, TaskType::CONTROL, 2, {17}, 32});
    scheduler.add_task({17, 10, 100, 5, 100, TaskType::ACTUATOR, 2, {}, 64}); // Aggregator: 64KB

    // GROUP B — Split Inference Layer (Period: 150ms)

    scheduler.add_task({18, 5, 150, 12, 150, TaskType::CONTROL, 0, {19}, 128}); // Layer 1: 128KB
    scheduler.add_task({19, 5, 150, 3,  150, TaskType::CONTROL, 1, {20}, 64});  
    scheduler.add_task({20, 6, 150, 2,  150, TaskType::CONTROL, 1, {21}, 32});  
    scheduler.add_task({21, 6, 150, 12, 150, TaskType::CONTROL, 1, {22}, 128}); // Layer 2: 128KB
    scheduler.add_task({22, 7, 150, 2,  150, TaskType::CONTROL, 1, {23}, 32});  
    scheduler.add_task({23, 7, 150, 12, 150, TaskType::CONTROL, 1, {24}, 128}); // Layer 3: 128KB
    scheduler.add_task({24, 9, 150, 4,  150, TaskType::ACTUATOR,1, {25}, 64});  
    scheduler.add_task({25, 6, 150, 3,  150, TaskType::CONTROL, 1, {}, 32});    

    // GROUP C — Distributed Training + Backprop (Period: 300ms)
    scheduler.add_task({26, 5, 300, 5, 300, TaskType::CONTROL, 0, {29}, 64});
    scheduler.add_task({27, 6, 300, 5, 300, TaskType::CONTROL, 0, {30}, 64});
    scheduler.add_task({28, 7, 300, 5, 300, TaskType::CONTROL, 0, {31}, 64});

    scheduler.add_task({29, 5, 300, 15, 300, TaskType::CONTROL, 1, {30}, 128});
    scheduler.add_task({30, 6, 300, 15, 300, TaskType::CONTROL, 2, {31}, 128});
    scheduler.add_task({31, 7, 300, 15, 300, TaskType::CONTROL, 2, {32}, 128});

    scheduler.add_task({32, 9, 300, 6,  300, TaskType::CONTROL, 1, {33}, 64});         
    scheduler.add_task({33, 7, 300, 10, 300, TaskType::CONTROL, 1, {34, 38}, 128});     
    scheduler.add_task({34, 6, 300, 10, 300, TaskType::CONTROL, 1, {35, 37}, 128});     
    scheduler.add_task({35, 5, 300, 10, 300, TaskType::CONTROL, 1, {36}, 128});         

    scheduler.add_task({36, 5, 300, 4, 300, TaskType::ACTUATOR, 1, {39}, 32});
    scheduler.add_task({37, 6, 300, 4, 300, TaskType::ACTUATOR, 1, {39}, 32});
    scheduler.add_task({38, 7, 300, 4, 300, TaskType::ACTUATOR, 1, {39}, 32});
    scheduler.add_task({39, 10, 300, 5, 300, TaskType::ACTUATOR, 3, {40}, 128}); // Checkpoint: 128KB       
    scheduler.add_task({40, 11, 300, 2, 300, TaskType::CONTROL, 1, {}, 16});           

    // GROUP D — High-Priority Interrupts (Period: 10ms)
    // Notice these safety-critical tasks use almost no memory! (8KB)
    scheduler.add_task({41, 0,  10, 1, 10, TaskType::SENSOR, 0, {}, 8}); 
    scheduler.add_task({42, 4,  10, 1, 10, TaskType::SENSOR, 0, {}, 8}); 
    scheduler.add_task({43, 11, 10, 2, 10, TaskType::SENSOR, 0, {}, 8}); 
    scheduler.add_task({44, 8,  10, 1, 10, TaskType::SENSOR, 0, {}, 8}); 

    scheduler.check_schedulability();
    // RUN 1: RMS
    scheduler.run(sim_time, Policy::RMS);
    scheduler.print_metrics();
    scheduler.export_gantt_csv("trace_rms.csv");

    // RUN 2: EDF
    scheduler.run(sim_time, Policy::EDF);
    scheduler.print_metrics();
    scheduler.export_gantt_csv("trace_edf.csv");


    return 0;
}