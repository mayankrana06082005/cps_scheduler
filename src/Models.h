#pragma once
#include <string>
#include <memory>
#include <vector>

enum class TaskType { SENSOR, CONTROL, ACTUATOR };
enum class JobState { WAITING, READY, RUNNING, BLOCKED, COMPLETED, MISSED };

struct Task {
    int id;
    int node_id;           
    int period;
    int wcet;              
    int relative_deadline;
    TaskType type;
    
    int num_predecessors;              
    std::vector<int> successor_ids;    
    
    int memory_required; 
};

struct Job {
    int id;                 
    int task_id;            
    int node_id;            
    int release_time;       
    int absolute_deadline;  
    int remaining_time;     
    JobState state;
    
    int parent_period;      
    int pending_dependencies; 
    int last_start_time;  
    
    int memory_required; 
    bool memory_allocated; 
};

struct NetworkMessage {
    int source_node;
    int dest_node;
    int dest_task_id;
    int arrival_time;  
};

struct GanttRecord {
    int node_id;
    int task_id;
    int start_time;
    int end_time;
};

using JobPtr = std::shared_ptr<Job>;