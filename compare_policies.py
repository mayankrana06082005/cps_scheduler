import subprocess
import re
import matplotlib.pyplot as plt
import numpy as np
from collections import Counter

def run_comparative_monte_carlo(num_runs=100):
    print(f"--- D-RTSS: RMS vs EDF Comparative Analysis ---")
    print(f"Executing C++ Engine {num_runs} times with Gaussian Jitter...\n")
    
    rms_misses = []
    edf_misses = []
    
    for i in range(num_runs):
        if (i + 1) % 10 == 0:
            print(f"Progress: {i + 1}/{num_runs} simulations complete...")
            
        # Execute the compiled C++ simulator
        result = subprocess.run(['./build/simulator'], capture_output=True, text=True)
        
        # We expect exactly TWO instances of "Missed: [number]" in the output
        # Index 0 is RMS, Index 1 is EDF (because of the order in main.cpp)
        matches = re.findall(r'Missed:\s+(\d+)', result.stdout)
        
        if len(matches) >= 2:
            rms_misses.append(int(matches[0]))
            edf_misses.append(int(matches[1]))
        else:
            print("Error parsing C++ output. Did it run both policies?")
            break

    # Statistical Analysis
    rms_failures = sum(1 for m in rms_misses if m > 0)
    edf_failures = sum(1 for m in edf_misses if m > 0)
    
    print("\n=== Comparative Results ===")
    print(f"Total Simulation Epochs: {num_runs}")
    print(f"RMS Failure Probability: {(rms_failures/num_runs)*100:.1f}%")
    print(f"EDF Failure Probability: {(edf_failures/num_runs)*100:.1f}%\n")
    
    # --- Generate the Side-by-Side Histogram ---
    rms_counts = Counter(rms_misses)
    edf_counts = Counter(edf_misses)
    
    # Get the maximum number of misses to size the X-axis
    max_misses = max(max(rms_misses), max(edf_misses), 1)
    x_indices = np.arange(max_misses + 1)
    
    rms_y = [rms_counts.get(x, 0) for x in x_indices]
    edf_y = [edf_counts.get(x, 0) for x in x_indices]
    
    plt.figure(figsize=(12, 6))
    bar_width = 0.35
    
    plt.bar(x_indices - bar_width/2, rms_y, bar_width, label='RMS (Static Priority)', color='salmon', edgecolor='black')
    plt.bar(x_indices + bar_width/2, edf_y, bar_width, label='EDF (Dynamic Priority)', color='skyblue', edgecolor='black')
    
    plt.title('Schedulability Analysis: RMS vs EDF under Network Overload', fontsize=15)
    plt.xlabel('Number of Deadline Misses per Run', fontsize=12)
    plt.ylabel('Frequency (Out of {} runs)'.format(num_runs), fontsize=12)
    plt.xticks(x_indices)
    plt.legend(fontsize=12)
    plt.grid(axis='y', linestyle='--', alpha=0.7)
    
    plt.savefig("policy_comparison.png", dpi=300)
    print("Comparison graph saved to: policy_comparison.png")

if __name__ == "__main__":
    run_comparative_monte_carlo(num_runs=100)