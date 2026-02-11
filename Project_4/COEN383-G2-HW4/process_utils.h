#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "process.h"
#include "simulation_utils.h"

void generate_processes(Process processes[], int seed)
{
    srand(seed); // each of the 5 runs uses a different seed

    int sizes[] = {5, 11, 17, 31};

    for (int i = 0; i < NUM_PROCESSES; i++)
    {
        processes[i].id = i;
        sprintf(processes[i].name, "P%d", i);

        processes[i].size_pages = sizes[rand() % 4];

        // Generate float values
        processes[i].arrival_time = ((double)rand() / RAND_MAX) * 60.0; //  random double in [0, 60)
        processes[i].service_time = (rand() % 5) + 1;                   // random int from {1, 2, 3, 4, 5}
        processes[i].start_time = -1.0;
        processes[i].currentPage = 0;
        processes[i].pages_in_memory = 0;
        processes[i].completion_time = -1.0;

        // Initialize all page_table entries to -1
        for (int j = 0; j < 31; j++)
        {
            processes[i].page_table[j] = -1;
        }
    }

    // Sort by arrival time
    for (int i = 0; i < NUM_PROCESSES - 1; i++)
    {
        for (int j = 0; j < NUM_PROCESSES - i - 1; j++)
        {
            if (processes[j].arrival_time > processes[j + 1].arrival_time)
            {
                Process temp = processes[j];
                processes[j] = processes[j + 1];
                processes[j + 1] = temp;
            }
        }
    }
}

void print_processes(Process processes[], int num_processes)
{
    printf("\nProcess Details:\n");
    printf("Name\tArrival\tSize\tService\n");
    printf("--------------------------------------------\n");
    for (int i = 0; i < num_processes; i++)
    {
        printf("%s\t%.1f\t%d\t%d\n",
               processes[i].name,
               processes[i].arrival_time,
               processes[i].size_pages,
               processes[i].service_time);
    }
}

FreePageNode *init_free_list(int num_frames)
{
    FreePageNode *head = NULL;
    FreePageNode *tail = NULL;

    for (int i = 0; i < num_frames; i++)
    {
        FreePageNode *node = (FreePageNode *)malloc(sizeof(FreePageNode));
        node->frame_number = i;
        node->next = NULL;

        if (head == NULL)
        {
            head = node;
            tail = node;
        }
        else
        {
            tail->next = node;
            tail = node;
        }
    }

    return head;
}
