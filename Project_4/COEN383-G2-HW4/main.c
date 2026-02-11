#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include "process.h"
#include "process_utils.h"
#include "simulation_utils.h"

#define MIN_FREE_PAGES 4
#define REFERENCE_INTERVAL 0.1 // 100 msec in seconds
#define SIMULATION_TIME 60.0   // 1 minute

// Page replacement algorithms
typedef enum
{
    FIFO,
    LRU,
    LFU,
    MFU,
    RANDOM
} ReplacementAlgo;

const char *algo_names[] = {"FIFO", "LRU", "LFU", "MFU", "RANDOM"};

// Statistics
typedef struct
{
    int hits;
    int misses;
    int processes_swapped_in;
} Statistics;

// Global memory state
PageFrame memory[TOTAL_PAGES];
int free_page_count = TOTAL_PAGES;

// Function prototypes
int get_next_page(int current_page, int process_size);
int find_victim_page(ReplacementAlgo algo, double current_time);
void print_memory_map(Process processes[], int num_processes);
void simulate(Process processes[], ReplacementAlgo algo, Statistics *stats, int run_num, int print_details);
int allocate_initial_page(Process *proc, int proc_id);
void deallocate_process_pages(Process *proc);

// Get next page reference based on locality of reference
int get_next_page(int current_page, int process_size)
{
    int r = rand() % 11; // 0 to 10
    int next_page;

    if (r < 7)
    { // 70% probability: delta = -1, 0, or +1
        int delta = (rand() % 3) - 1; // -1, 0, or 1
        next_page = current_page + delta;

        // Wrap around
        if (next_page < 0)
            next_page = process_size - 1;
        if (next_page >= process_size)
            next_page = 0;
    }
    else
    { // 30% probability: |delta| >= 2
        // Generate j such that |i - j| >= 2
        int valid_pages[31];
        int count = 0;

        for (int j = 0; j < process_size; j++)
        {
            int delta = abs(j - current_page);
            if (delta >= 2 && delta <= process_size - 2)
            {
                valid_pages[count++] = j;
            }
        }

        if (count > 0)
        {
            next_page = valid_pages[rand() % count];
        }
        else
        {
            next_page = current_page;
        }
    }

    return next_page;
}

// Find victim page to evict based on replacement algorithm
int find_victim_page(ReplacementAlgo algo, double current_time)
{
    int victim = -1;

    switch (algo)
    {
    case FIFO:
    {
        // Find page with earliest load time
        double earliest_time = current_time + 1;
        for (int i = 0; i < TOTAL_PAGES; i++)
        {
            if (memory[i].process_id != -1 && memory[i].load_time < earliest_time)
            {
                earliest_time = memory[i].load_time;
                victim = i;
            }
        }
        break;
    }
    case LRU:
    {
        // Find page with least recent access time
        double least_recent = current_time + 1;
        for (int i = 0; i < TOTAL_PAGES; i++)
        {
            if (memory[i].process_id != -1 && memory[i].last_access_time < least_recent)
            {
                least_recent = memory[i].last_access_time;
                victim = i;
            }
        }
        break;
    }
    case LFU:
    {
        // Find page with least access count
        int min_count = 999999;
        for (int i = 0; i < TOTAL_PAGES; i++)
        {
            if (memory[i].process_id != -1 && memory[i].access_count < min_count)
            {
                min_count = memory[i].access_count;
                victim = i;
            }
        }
        break;
    }
    case MFU:
    {
        // Find page with most access count
        int max_count = -1;
        for (int i = 0; i < TOTAL_PAGES; i++)
        {
            if (memory[i].process_id != -1 && memory[i].access_count > max_count)
            {
                max_count = memory[i].access_count;
                victim = i;
            }
        }
        break;
    }
    case RANDOM:
    {
        // Pick random occupied page
        int occupied[TOTAL_PAGES];
        int count = 0;
        for (int i = 0; i < TOTAL_PAGES; i++)
        {
            if (memory[i].process_id != -1)
            {
                occupied[count++] = i;
            }
        }
        if (count > 0)
        {
            victim = occupied[rand() % count];
        }
        break;
    }
    }

    return victim;
}

// Print memory map
void print_memory_map(Process processes[], int num_processes)
{
    for (int i = 0; i < TOTAL_PAGES; i++)
    {
        if (memory[i].process_id == -1)
        {
            printf(".");
        }
        else
        {
            // Use single character from process name
            int proc_id = memory[i].process_id;
            if (proc_id < 10)
            {
                printf("%d", proc_id);
            }
            else if (proc_id < 36)
            {
                printf("%c", 'A' + (proc_id - 10));
            }
            else
            {
                printf("*");
            }
        }
    }
}

// Allocate initial page (page 0) for a process
int allocate_initial_page(Process *proc, int proc_id)
{
    if (free_page_count < MIN_FREE_PAGES)
        return 0;

    // Find a free frame
    int frame = -1;
    for (int i = 0; i < TOTAL_PAGES; i++)
    {
        if (memory[i].process_id == -1)
        {
            frame = i;
            break;
        }
    }

    if (frame == -1)
        return 0;

    // Allocate page 0
    memory[frame].process_id = proc_id;
    memory[frame].page_number = 0;
    memory[frame].last_access_time = proc->start_time;
    memory[frame].access_count = 1;
    memory[frame].load_time = proc->start_time;

    proc->page_table[0] = frame;
    proc->pages_in_memory = 1;
    free_page_count--;

    return 1;
}

// Deallocate all pages of a process
void deallocate_process_pages(Process *proc)
{
    for (int i = 0; i < proc->size_pages; i++)
    {
        if (proc->page_table[i] != -1)
        {
            int frame = proc->page_table[i];
            memory[frame].process_id = -1;
            memory[frame].page_number = -1;
            proc->page_table[i] = -1;
            free_page_count++;
        }
    }
    proc->pages_in_memory = 0;
}

// Main simulation function
void simulate(Process processes[], ReplacementAlgo algo, Statistics *stats, int run_num, int print_details)
{
    // Initialize memory
    for (int i = 0; i < TOTAL_PAGES; i++)
    {
        memory[i].process_id = -1;
        memory[i].page_number = -1;
        memory[i].last_access_time = 0.0;
        memory[i].access_count = 0;
        memory[i].load_time = 0.0;
    }
    free_page_count = TOTAL_PAGES;

    stats->hits = 0;
    stats->misses = 0;
    stats->processes_swapped_in = 0;

    double current_time = 0.0;
    int next_process_idx = 0;
    int running_processes[NUM_PROCESSES];
    int num_running = 0;
    int reference_count = 0;

    while (current_time <= SIMULATION_TIME)
    {
        // Try to admit new processes
        while (next_process_idx < NUM_PROCESSES &&
               processes[next_process_idx].arrival_time <= current_time &&
               free_page_count >= MIN_FREE_PAGES)
        {
            Process *proc = &processes[next_process_idx];
            proc->start_time = current_time;

            if (allocate_initial_page(proc, proc->id))
            {
                running_processes[num_running++] = next_process_idx;
                stats->processes_swapped_in++;

                if (print_details && stats->processes_swapped_in <= 10)
                {
                    printf("%.2f\t%s\tEnter\t%d\t%d\t", current_time, proc->name,
                           proc->size_pages, proc->service_time);
                    print_memory_map(processes, NUM_PROCESSES);
                    printf("\n");
                }
            }
            next_process_idx++;
        }

        // Process memory references for running processes
        for (int i = 0; i < num_running; i++)
        {
            int proc_idx = running_processes[i];
            Process *proc = &processes[proc_idx];

            // Check if process should complete
            if (current_time >= proc->start_time + proc->service_time)
            {
                proc->completion_time = current_time;
                deallocate_process_pages(proc);

                if (print_details && i < 5)
                {
                    printf("%.2f\t%s\tExit\t%d\t%d\t", current_time, proc->name,
                           proc->size_pages, proc->service_time);
                    print_memory_map(processes, NUM_PROCESSES);
                    printf("\n");
                }

                // Remove from running list
                for (int j = i; j < num_running - 1; j++)
                {
                    running_processes[j] = running_processes[j + 1];
                }
                num_running--;
                i--;
                continue;
            }

            // Generate memory reference every 100ms
            if (fmod(current_time - proc->start_time, REFERENCE_INTERVAL) < 0.01)
            {
                int next_page = get_next_page(proc->currentPage, proc->size_pages);
                proc->currentPage = next_page;

                // Check if page is in memory
                int frame = proc->page_table[next_page];
                int page_in_memory = (frame != -1);

                if (page_in_memory)
                {
                    // Hit
                    stats->hits++;
                    memory[frame].last_access_time = current_time;
                    memory[frame].access_count++;

                    if (print_details && reference_count < 100)
                    {
                        printf("%.2f\t%s\t%d\tYes\t-\n", current_time, proc->name, next_page);
                        reference_count++;
                    }
                }
                else
                {
                    // Miss
                    stats->misses++;

                    // Find free frame or victim
                    int victim_frame = -1;
                    int victim_proc_id = -1;
                    int victim_page_num = -1;

                    // First try to find free frame
                    for (int f = 0; f < TOTAL_PAGES; f++)
                    {
                        if (memory[f].process_id == -1)
                        {
                            victim_frame = f;
                            break;
                        }
                    }

                    // If no free frame, evict a page
                    if (victim_frame == -1)
                    {
                        victim_frame = find_victim_page(algo, current_time);
                        if (victim_frame != -1)
                        {
                            victim_proc_id = memory[victim_frame].process_id;
                            victim_page_num = memory[victim_frame].page_number;

                            // Update victim process page table
                            if (victim_proc_id >= 0 && victim_proc_id < NUM_PROCESSES)
                            {
                                processes[victim_proc_id].page_table[victim_page_num] = -1;
                            }
                        }
                    }
                    else
                    {
                        free_page_count--;
                    }

                    // Load new page
                    if (victim_frame != -1)
                    {
                        memory[victim_frame].process_id = proc->id;
                        memory[victim_frame].page_number = next_page;
                        memory[victim_frame].last_access_time = current_time;
                        memory[victim_frame].access_count = 1;
                        memory[victim_frame].load_time = current_time;

                        proc->page_table[next_page] = victim_frame;
                        proc->pages_in_memory++;

                        if (print_details && reference_count < 100)
                        {
                            if (victim_proc_id != -1)
                            {
                                printf("%.2f\t%s\t%d\tNo\tP%d-pg%d\n",
                                       current_time, proc->name, next_page,
                                       victim_proc_id, victim_page_num);
                            }
                            else
                            {
                                printf("%.2f\t%s\t%d\tNo\t-\n", current_time, proc->name, next_page);
                            }
                            reference_count++;
                        }
                    }
                }
            }
        }

        current_time += 0.01; // Advance by 10ms
    }
}

int main()
{
    printf("=== Memory Management Simulation ===\n\n");

    // Run simulation for each algorithm
    for (int algo = FIFO; algo <= RANDOM; algo++)
    {
        printf("\n========================================\n");
        printf("Algorithm: %s\n", algo_names[algo]);
        printf("========================================\n\n");

        int total_hits = 0;
        int total_misses = 0;
        int total_swapped_in = 0;

        for (int run = 0; run < NUM_RUNS; run++)
        {
            Process processes[NUM_PROCESSES];
            generate_processes(processes, 1000 + run * 100 + algo * 500);

            Statistics stats;
            int print_details = (run == 0 && algo == FIFO); // Print details for first run of FIFO

            if (print_details)
            {
                printf("Sample output for first 100 references (Run 1):\n");
                printf("Time\tProc\tPage\tInMem\tEvicted\n");
                printf("------------------------------------------------\n");
            }

            simulate(processes, algo, &stats, run, print_details);

            total_hits += stats.hits;
            total_misses += stats.misses;
            total_swapped_in += stats.processes_swapped_in;

            if (print_details)
            {
                printf("\n");
            }

            printf("Run %d: Hits=%d, Misses=%d, Hit Ratio=%.3f, Processes Swapped In=%d\n",
                   run + 1, stats.hits, stats.misses,
                   (double)stats.hits / (stats.hits + stats.misses),
                   stats.processes_swapped_in);
        }

        // Print averages
        double avg_hit_ratio = (double)total_hits / (total_hits + total_misses);
        double avg_swapped_in = (double)total_swapped_in / NUM_RUNS;

        printf("\n--- %s Algorithm Summary ---\n", algo_names[algo]);
        printf("Average Hit Ratio: %.3f\n", avg_hit_ratio);
        printf("Average Processes Swapped In: %.2f\n", avg_swapped_in);
        printf("Total Hits: %d\n", total_hits);
        printf("Total Misses: %d\n", total_misses);
    }

    printf("\n=== Simulation Complete ===\n");

    return 0;
}
