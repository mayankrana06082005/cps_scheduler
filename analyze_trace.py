import pandas as pd
import os

def analyze_policy(filepath, policy_name):
    print(f"  ANALYZING POLICY: {policy_name}")
    
    if not os.path.exists(filepath):
        print(f"Error: Could not find {filepath}. Run the C++ simulator first.")
        return

    df = pd.read_csv(filepath)
    if df.empty:
        print("Trace file is empty.")
        return

    # ACTUAL CPU UTILIZATION ANALYSIS
    sim_time = df['End'].max()
    print(f"\n1. ACTUAL CPU UTILIZATION (Over {sim_time}ms Simulation)")
    
    # Calculate duration of every execution chunk
    df['Duration'] = df['End'] - df['Start']
    
    # Group by Node and sum the active time
    node_active_time = df.groupby('Node')['Duration'].sum()
    
    for node_id in range(12): 
        active = node_active_time.get(node_id, 0)
        utilization = (active / sim_time) * 100
        print(f"   Node {node_id:2}: {utilization:5.1f}% utilization")

    # END-TO-END DAG LATENCY (Critical Path Analysis)
    print("\n2. CRITICAL PATH LATENCY ANALYSIS (Task 1 -> Task 17)")
    
    # Task 1 started its first cycle
    t1_starts = df[df['Task'] == 1]['Start'].sort_values().tolist()
    
    # Task 17 finished its first cycle
    t17_ends = df[df['Task'] == 17]['End'].sort_values().tolist()
    
    if t1_starts and t17_ends:
        first_epoch_latency = t17_ends[0] - t1_starts[0]
        print(f"   Epoch 1 Latency: {first_epoch_latency}ms")
        
        print(f"   Theoretical Minimum (Zero Delay/Interference): ~35ms")
        print(f"   Actual Latency Penalty: {first_epoch_latency - 35}ms of system overhead/jitter.")
    else:
        print("   Could not trace path. Did Task 17 miss its deadline or fail to execute?")

if __name__ == "__main__":
    print("D-RTSS: System Architecture Analysis ")
    analyze_policy("build/trace_rms.csv", "RATE MONOTONIC SCHEDULING (RMS)")
    analyze_policy("build/trace_edf.csv", "EARLIEST DEADLINE FIRST (EDF)")