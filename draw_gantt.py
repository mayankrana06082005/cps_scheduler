import pandas as pd
import matplotlib.pyplot as plt
import matplotlib.patches as mpatches
import os

def draw_chart(filepath, output_filename, title):
    if not os.path.exists(filepath):
        print(f"Could not find {filepath}. Did you run the simulator first?")
        return

    df = pd.read_csv(filepath)
    if df.empty:
        print(f"Trace file {filepath} is empty.")
        return

    fig, ax = plt.subplots(figsize=(16, 10))

    tasks = sorted(df['Task'].unique())
    colors = plt.cm.get_cmap('tab20', max(tasks) + 1)
    task_colors = {task: colors(task) for task in tasks}

    # Draw horizontal bars for each execution chunk
    for i, row in df.iterrows():
        node = row['Node']
        duration = row['End'] - row['Start']
        
        ax.barh(node, duration, left=row['Start'], 
                color=task_colors[row['Task']], edgecolor='black', linewidth=0.5)

    # Formatting the Chart
    max_node = df['Node'].max() if not df.empty else 11
    ax.set_yticks(range(max_node + 1))
    ax.set_ylabel('Node ID (Hardware CPUs)', fontsize=12)
    ax.set_xlabel('Time (milliseconds)', fontsize=12)
    ax.set_title(title, fontsize=16)

    # Create a Legend
    handles = [mpatches.Patch(color=task_colors[t], label=f'Task {t}') for t in tasks]
    ax.legend(handles=handles, bbox_to_anchor=(1.02, 1), loc='upper left', ncol=2, title="Task Legend")

    plt.grid(axis='x', linestyle='--', alpha=0.7)
    plt.tight_layout()
    plt.savefig(output_filename, dpi=300)
    plt.close() 
    print(f"Success! Gantt chart exported as {output_filename}")

if __name__ == "__main__":
    print("--- D-RTSS: Generating Gantt Charts ---")
    
    draw_chart("build/trace_rms.csv", "gantt_chart_rms.png", "Distributed AI Pipeline: RMS Policy")
    
    draw_chart("build/trace_edf.csv", "gantt_chart_edf.png", "Distributed AI Pipeline: EDF Policy")