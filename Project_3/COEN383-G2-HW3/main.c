#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "customers.h"
#include "simulation_utils.c"
#include "seller.h"

// Main Idea: 10 ticket sellers to 100 seats concert during one hour. Each ticket seller has their own queue for buyers.

// Global variables
#define NUM_SELLERS 10
int queueSizes[NUM_SELLERS];   // # of customers each seller has
int nextCustomer[NUM_SELLERS]; // index of next customer to be served per seller
int currentTime = 0;
int seatChart[10][10] = {0}; // 0 = available, 1 = taken
int avalailableSeats = 100;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// A thread can find its own-id: thread-id = pthread_self();
// How a thread can find out about its own customers’ queue?
// option-1: thread can find out its own thread-id and check the tid[] array to find the index
// in the workload array where each entry is the head of the customers linked list.
// option-2: when creating the thread, pass the last argument as the head of that thread’s
// customers linked list…
// option-3: when creating the thread, pass a struct that includes the index “i” and seller_type
// seller thread to serve one time slice (1 minute)

// Returns 1 if seat assigned, 0 if sold out
// Updates seatRow and seatCol if successful
int assignSeat(char sellerType, int *seatRow, int *seatCol)
{
    // Find next available seat based on seller type
    // H: starts row 0, goes forward
    // L: starts row 9, goes backward
    // M: starts row 4, zigzags (4, 5, 3, 6, 2, 7, 1, 8, 0, 9)

    // Check if seat available
    //  Mark seat as taken
    // Return success/failure
}

// void printSeatingChart(){}

// void printEvent(int time, char *message) {}

void calculateStatistics()
{
    // Calculate and print statistics:
    // - How many H/M/L customers got seats
    // - How many were turned away
    // - Average response time per customer for a given seller type
    // - Average turnaround time per customer for a given seller type
    // - Average throughput per seller type
}

// NOT COMPLETE
// currently just prints out it started, waits once, prints its exiting but doesn't process any customers
void *sell(void *s_t)
{
    // while (have more work to do){___________
    Seller *info = (Seller *)s_t; //  char *sellerType = (char *)s_t; // get seller type
    int myID = info->sellerID;
    char myType = info->sellerType;
    int myNumber = info->sellerNumber;

    printf("Seller %c%d (ID: %d) started\n", myType, myNumber, myID);

    // Wait for wakeup
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex); // wait until next buyer comes for this seller
    pthread_mutex_unlock(&mutex);

    // Serve any buyer available in this seller queue that is ready
    // now to buy ticket till done with all relevant buyers in their queue
    printf("Seller %c%d woke up and exiting\n", myType, myNumber);
    // }______________
    return NULL;
}

void wakeup_all_seller_threads()
{
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[])
{
    // Get N from the user via command line
    if (argc != 2)
    {
        printf("Usage: %s <number_of_customers>\n", argv[0]);
        return 1;
    }
    int N = atoi(argv[1]);

    // Seed random number generator **
    srand(time(NULL));

    int i;              // for loop index
    pthread_t tids[10]; // thread ids for 10 seller threads

    // Create necessary data structures for the simulator.
    Customer queues[NUM_SELLERS][N];

    // Create buyers list for each seller ticket queue based on the
    // N value within an hour and have them in the seller queue.
    // Generate customers for all sellers
    generateCustomers(queues[0], N, 'H', 1);
    generateCustomers(queues[1], N, 'M', 1);
    generateCustomers(queues[2], N, 'M', 2);
    generateCustomers(queues[3], N, 'M', 3);
    generateCustomers(queues[4], N, 'L', 1);
    generateCustomers(queues[5], N, 'L', 2);
    generateCustomers(queues[6], N, 'L', 3);
    generateCustomers(queues[7], N, 'L', 4);
    generateCustomers(queues[8], N, 'L', 5);
    generateCustomers(queues[9], N, 'L', 6);

    // Debugging
    // printQueue(queues[0], N, 'H', 1); // H seller
    // printQueue(queues[1], N, 'M', 1); // M1 seller
    // printQueue(queues[4], N, 'L', 1); // L1 seller

    // Generate the sellers
    Seller sellers[10];

    // Initialize seller info for identification
    // H
    sellers[0].sellerID = 0;
    sellers[0].sellerType = 'H';
    sellers[0].sellerNumber = 1;

    // M
    for (i = 1; i <= 3; i++)
    {
        sellers[i].sellerID = i;
        sellers[i].sellerType = 'M';
        sellers[i].sellerNumber = i; // M1, M2, M3
    }

    // L
    for (i = 4; i < 10; i++)
    {
        sellers[i].sellerID = i;
        sellers[i].sellerType = 'L';
        sellers[i].sellerNumber = i - 3; // L1-L6
    }

    // Assume 10 threads, each represents a ticket seller: H1, M1, M2, M3, L1, L2, L3, L4, L5, L6.
    printf("Creating 10 threads representing the 10 sellers...\n");
    for (i = 0; i < 10; i++)
    {
        pthread_create(&tids[i], NULL, sell, &sellers[i]);
    }

    // wakeup all seller threads
    printf("Waking up all seller threads...\n");
    wakeup_all_seller_threads();

    /* Implement the simulation logic here???
        For each minute 0-60:
            a. Check which customers have arrived
            b. Sellers serve customers (assign seats)
            c. Print events as they happen
            d. Update customer records
        All threads finish when:
            - All customers served OR
            - Hour is up OR
            - Seats sold out
        Calculate and print statistics
    */

    // wait for all seller threads to exit
    printf("Waiting for all seller threads to exit...\n");
    for (i = 0; i < 10; i++)
        pthread_join(&tids[i], NULL);

    // Printout simulation results

    /*

    11. Print final statistics:
        - How many H/M/L customers got seats
        - How many were turned away
        - Average response time per seller type
        - Average turnaround time per seller type
        - Average throughput per seller type

    12. Cleanup and exit

    */

    printf("Simulation complete.\n");

    return 0;
}
