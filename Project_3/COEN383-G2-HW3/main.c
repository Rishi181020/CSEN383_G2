#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "customers.h"
#include "simulation_utils.c"
#include "seller.h"

// Main Idea: 10 ticket sellers to 100 seats concert during one hour. Each ticket seller has their own queue for buyers.

// Function to assign seat based on seller type
int assignSeat(char sellerType, char *customerID, int *seatRow, int *seatCol)
{
    if (availableSeats <= 0)
    {
        return 0; // No seats available
    }

    int searchOrder[10];

    // Define row search order for convenience based on seller type
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

    // Search for empty seat based on search order
    for (int i = 0; i < 10; i++)
    {
        int row = searchOrder[i];
        for (int col = 0; col < 10; col++)
        {
            if (strcmp(seatChart[row][col], "----") == 0) // Empty seat found
            {
                strcpy(seatChart[row][col], customerID); // Assign customerID to seat
                *seatRow = row;
                *seatCol = col;
                availableSeats--; // Decrease available seats
                return 1;
            }
        }
    }

    return 0; // No seat found
}

// Each seller serves their own queue
void *sell(void *s_t)
{
    // Extract seller info for convenience
    Seller *info = (Seller *)s_t;
    int myId = info->sellerID;
    char sellerType = info->sellerType;
    int myNumber = info->sellerNumber;

    // Message buffer for printing
    char msg[200];

    // Track when seller is free
    int sellerFree = 0;

    // Serve customers in the queue
    while (nextCustomer[myId] < queueSizes[myId])
    {
        pthread_mutex_lock(&mutex); // Lock for synchronization

        Customer *customer = &queues[myId][nextCustomer[myId]];

        // Wait until customer arrives and seller is free, but only up to 60 min
        while ((customer->arrivalTime > currentTime) || (currentTime < sellerFree))
        {
            if (currentTime > 60)
            {
                break; // stop waiting if simulation ended
            }
            pthread_cond_wait(&cond, &mutex);
        }

        if (currentTime > 60 && customer->arrivalTime > 60)
        {
            // Customer arrived too late -> turn away
            customer->gotSeat = 0;
            customer->startTime = -1;
            customer->endTime = 60;
            nextCustomer[myId]++;
            pthread_mutex_unlock(&mutex);
            continue;
        }

        // Set startTime correctly
        customer->startTime = MAX(currentTime, sellerFree);

        sprintf(msg, "Customer %s arrives at seller %c%d's queue",
                customer->customerID, sellerType, myNumber);
        printEvent(customer->arrivalTime, msg);

        // Now serve the customer
        int seatRow = -1, seatCol = -1;
        int seatAssigned = 0;

        // Assign seat safely inside mutex
        if (availableSeats > 0)
        {
            seatAssigned = assignSeat(sellerType, customer->customerID, &seatRow, &seatCol);
        }

        if (seatAssigned)
        {
            // Seat assigned successfully

            customer->seatRow = seatRow;
            customer->seatCol = seatCol;
            customer->gotSeat = 1;
            customer->endTime = customer->startTime + customer->serviceTime;

            sellerFree = customer->endTime;

            sprintf(msg, "Seller %c%d assigns seat (%d,%d) to customer %s",
                    sellerType, myNumber, seatRow, seatCol, customer->customerID);
            printEvent(currentTime, msg);

            sprintf(msg, "Customer %s completes purchase (service: %d min)",
                    customer->customerID, customer->serviceTime);
            printEvent(customer->endTime, msg);
        }
        else
        {
            // Sold out
            customer->gotSeat = 0;
            customer->endTime = currentTime;

            sprintf(msg, "Customer %s turned away by %c%d - SOLD OUT",
                    customer->customerID, sellerType, myNumber);
            printEvent(currentTime, msg);
        }

        nextCustomer[myId]++; // Move to next customer
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
}

// Wake up all seller threads function provided in project description

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

    // Seed random number generator
    srand(time(NULL));

    pthread_t tids[10]; // thread ids for 10 seller threads

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

    // Generate the sellers
    Seller sellers[10];

    // Initialize seller info for identification
    // H
    sellers[0].sellerID = 0;
    sellers[0].sellerType = 'H';
    sellers[0].sellerNumber = 1;

    // M
    for (int i = 1; i <= 3; i++)
    {
        sellers[i].sellerID = i;
        sellers[i].sellerType = 'M';
        sellers[i].sellerNumber = i; // M1, M2, M3
    }

    // L
    for (int i = 4; i < 10; i++)
    {
        sellers[i].sellerID = i;
        sellers[i].sellerType = 'L';
        sellers[i].sellerNumber = i - 3; // L1-L6
    }

    // Assume 10 threads, each represents a ticket seller: H1, M1, M2, M3, L1, L2, L3, L4, L5, L6.
    printEvent(currentTime, "Creating 10 threads representing the 10 sellers...");
    for (int i = 0; i < 10; i++)
    {
        pthread_create(&tids[i], NULL, sell, &sellers[i]);
    }

    usleep(100000); // Allow threads to start

    // Simulate time —  60 minutes
    for (; currentTime <= 60; currentTime++)
    {
        pthread_cond_broadcast(&cond); // wake up all seller threads

        usleep(50000); // Simulate 1 minute passing (50ms)

        pthread_mutex_lock(&mutex);
        int seats = availableSeats; // check available seats
        pthread_mutex_unlock(&mutex);

        if (seats == 0) // if sold out, end simulation early
        {
            char msg[100];
            sprintf(msg, "Concert SOLD OUT at minute %d!", currentTime);
            printEvent(currentTime, msg);
            break;
        }
    }

    // Signal all threads that simulation has ended
    pthread_mutex_lock(&mutex);
    currentTime = 61; // Set time past 60 to trigger thread exit
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&cond);

    usleep(100000); // Give threads time to exit

    printf("Waiting for all seller threads to exit...\n");
    for (int i = 0; i < 10; i++)
    {
        pthread_join(tids[i], NULL); // wait for each seller thread to finish
    }

    printSeatingChart();   // print final seating chart
    calculateStatistics(); // print statistics

    printf("Simulation complete.\n");

    return 0;
}
