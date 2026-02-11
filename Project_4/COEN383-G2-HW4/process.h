#ifndef PROCESS_H
#define PROCESS_H

typedef struct
{
    int id;
    char name[8];
    int size_pages; // process size in pages (5, 11, 17, and 31 MB)
    double arrival_time;
    int service_time; // randomly distributed service durations (1,2,3,4, or 5 seconds)
    double start_time;
    int currentPage;
    int page_table[31];
    int pages_in_memory;
    double completion_time;
} Process;

typedef struct FreePageNode
{
    int frame_number;
    struct FreePageNode *next;
} FreePageNode;

typedef struct PageFrame
{
    int process_id;
    int page_number;
    double last_access_time;
    int access_count;
    double load_time;
} PageFrame;

void generate_processes(Process processes[], int count);
void print_processes(Process processes[], int num_processes);
FreePageNode *init_free_list(int num_frames);
