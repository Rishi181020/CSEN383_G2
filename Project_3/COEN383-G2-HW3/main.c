#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "customers.h"
#include "simulation_utils.c"
#include "seller.h"

// Main Idea: 10 ticket sellers to 100 seats concert during one hour. Each ticket seller has their own queue for buyers.

// Global variables
#define NUM_SELLERS 10
#define MAX_CUSTOMERS 20
Customer queues[NUM_SELLERS][MAX_CUSTOMERS]; // queues for each seller
int queueSizes[NUM_SELLERS];                 // # of customers each seller has
int nextCustomer[NUM_SELLERS];               // index of next customer to be served per seller
int currentTime = 0;                         // to simulate time from 0 to 59 minutes
char seatChart[10][10][5];                   // 2D array to represent 100 seats, each can hold customerID or "----" (5 chars)
int availableSeats = 100;
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

int assignSeat(char sellerType, char *customerID, int *seatRow, int *seatCol)
{
    if (availableSeats <= 0)
    {
        return 0; // No seats available
    }

    int searchOrder[10];

    // Define row search order for convenience
    if (sellerType == 'H')
    {
        for (int i = 0; i < 10; i++)
            searchOrder[i] = i; // 0->9
    }
    else if (sellerType == 'L')
    {
        for (int i = 0; i < 10; i++)
            searchOrder[i] = 9 - i; // 9->0
    }
    else if (sellerType == 'M')
    {
        int order[] = {4, 5, 3, 6, 2, 7, 1, 8, 0, 9};
        for (int i = 0; i < 10; i++)
            searchOrder[i] = order[i]; // 4,5,3,6,2,7,1,8,0,9
    }
    else
    {
        return 0;
    }

    // Search for empty seat with search order
    for (int i = 0; i < 10; i++)
    {
        int row = searchOrder[i];
        for (int col = 0; col < 10; col++)
        {
            if (strcmp(seatChart[row][col], "----") == 0)
            {
                strcpy(seatChart[row][col], customerID);
                *seatRow = row;
                *seatCol = col;
                availableSeats--;
                return 1;
            }
        }
    }

    return 0; // No seat found
}

// NEED TO IMPLEMENT
void calculateStatistics()
{
    // Calculate and print statistics:
    // - How many H/M/L customers got seats
    // - How many were turned away
    // - Average response time per customer for a given seller type
    // - Average turnaround time per customer for a given seller type
    // - Average throughput per seller type
}

// NEED TO IMPLEMENT
// currently just prints out it started, waits once, prints its exiting but doesn't process any customers
void *sell(void *s_t)
{
    Seller *info = (Seller *)s_t;
    int myID = info->sellerID;

    // Wait for initial wakeup
    pthread_mutex_lock(&mutex);
    pthread_cond_wait(&cond, &mutex);
    pthread_mutex_unlock(&mutex);

    // Process all customers
    while (nextCustomer[myID] < queueSizes[myID] && availableSeats > 0)
    {
        Customer *c = &queues[myID][nextCustomer[myID]];

        // Wait until customer arrives
        while (currentTime < c->arrivalTime)
        {
            pthread_mutex_lock(&mutex);
            pthread_cond_wait(&cond, &mutex); // Sleep until time advances
            pthread_mutex_unlock(&mutex);
        }

        // Customer has arrived, serve them
        c->startTime = currentTime;

        // Assign seat (critical section)
        pthread_mutex_lock(&mutex);
        int row, col;
        int success = assignSeat(info->sellerType, c->customerID, &row, &col);
        if (success)
        {
            c->seatRow = row;
            c->seatCol = col;
            c->gotSeat = 1;
            c->endTime = currentTime + c->serviceTime;
        }
        pthread_mutex_unlock(&mutex);

        nextCustomer[myID]++;
    }

    return NULL; // Thread exits when done
}

void wakeup_all_seller_threads()
{
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

// Print the seating chart
void printSeatingChart()
{
    pthread_mutex_lock(&mutex);
    printf("\n========== SEATING CHART ==========\n");
    printf("    ");

    // Print column headers (0-9)
    for (int col = 0; col < 10; col++)
    {
        printf("Col%d ", col);
    }
    printf("\n");

    // Print separator line
    printf("    ");
    for (int col = 0; col < 10; col++)
    {
        printf("---- ");
    }
    printf("\n");

    // Print each row
    for (int row = 0; row < 10; row++)
    {
        printf("R%d: ", row); // Row label

        for (int col = 0; col < 10; col++)
        {
            printf("%-4s ", seatChart[row][col]); // Print customer ID or "----"
        }
        printf("\n");
    }

    printf("===================================\n");
    printf("Available seats: %d / 100\n\n", availableSeats);
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
    int N = atoi(argv[1]); // N customers for each sellers queue
    if (N > MAX_CUSTOMERS)
    {
        printf("Error: N exceeds maximum allowed customers (%d)\n", MAX_CUSTOMERS);
        return 1;
    }

    // Seed random number generator **
    srand(time(NULL));

    int i;              // for loop index
    pthread_t tids[10]; // thread ids for 10 seller threads

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

    for (int i = 0; i < NUM_SELLERS; i++)
    {
        queueSizes[i] = N;   // Each seller has N customers
        nextCustomer[i] = 0; // Start at first customer
    }

    // Initialize seating chart with empty seats "----"
    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 10; j++)
        {
            strcpy(seatChart[i][j], "----");
        }
    }

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
    printEvent(currentTime, "Creating 10 threads representing the 10 sellers...");
    for (i = 0; i < 10; i++)
    {
        pthread_create(&tids[i], NULL, sell, &sellers[i]);
    }

    // wakeup all seller threads
    printEvent(currentTime, "Waking up all seller threads...");
    wakeup_all_seller_threads();

    for (currentTime = 0; currentTime < 60; currentTime++)
    {
        pthread_cond_broadcast(&cond);

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
        */

        if (availableSeats == 0)
        {
            printf("All tickets sold out! Stopping simulation early.\n");
            break;
        }
    }
    // wait for all seller threads to exit
    printf("Waiting for all seller threads to exit...\n");
    for (i = 0; i < 10; i++)
    {
        pthread_join(&tids[i], NULL);
    }

    printSeatingChart(); // print final seating chart
    calculateStatistics();
    /*
    Print final statistics:
        - How many H/M/L customers got seats
        - How many were turned away
        - Average response time per seller type
        - Average turnaround time per seller type
        - Average throughput per seller type
    */

    printf("Simulation complete.\n");

    return 0;
}
