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
int availableSeats = 100;                    // total available seats left

// For synchronization
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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

void calculateStatistics()
{
    int H_turned = 0;
    int M_turned = 0;
    int L_turned = 0;
    int H_served = 0;
    int M_served = 0;
    int L_served = 0;
    double h_total_rt = 0;
    double m_total_rt = 0;
    double l_total_rt = 0;
    double h_total_tt = 0;
    double m_total_tt = 0;
    double l_total_tt = 0;

    // Iterate through all sellers and their queues
    for (int seller = 0; seller < NUM_SELLERS; seller++)
    {
        char sellerType;
        if (seller == 0)
        {
            sellerType = 'H';
        }
        else if (seller >= 1 && seller < 4)
        {
            sellerType = 'M';
        }
        else
        {
            sellerType = 'L';
        }

        // Process each customer in the seller's queue
        for (int i = 0; i < queueSizes[seller]; i++)
        {
            Customer *c = &queues[seller][i];
            if (c->gotSeat == 1)
            {
                int each_customer_rt = c->startTime - c->arrivalTime;
                int each_customer_tt = c->endTime - c->arrivalTime;
                if (sellerType == 'H')
                {
                    h_total_rt += each_customer_rt;
                    h_total_tt += each_customer_tt;
                    H_served++;
                }
                else if (sellerType == 'M')
                {
                    m_total_rt += each_customer_rt;
                    m_total_tt += each_customer_tt;
                    M_served++;
                }
                else
                {
                    l_total_rt += each_customer_rt;
                    l_total_tt += each_customer_tt;
                    L_served++;
                }
            }
            else
            {
                if (sellerType == 'H')
                {
                    H_turned++;
                }
                else if (sellerType == 'M')
                {
                    M_turned++;
                }
                else
                {
                    L_turned++;
                }
            }
        }
    }
    printf("High-Price Seller (H):\n");
    printf("  Customers served: %d\n", H_served);
    printf("  Customers turned away: %d\n", H_turned);
    if (H_served > 0)
    {
        printf("  Average response time: %.2f minutes\n", h_total_rt / H_served);
        printf("  Average turnaround time: %.2f minutes\n", h_total_tt / H_served);
        printf("  Throughput: %.2f customers/hour\n", (double)H_served); /// 60.0
    }
    printf("\n");

    // Medium-price sellers
    printf("Medium-Price Sellers (M1, M2, M3):\n");
    printf("  Customers served: %d\n", M_served);
    printf("  Customers turned away: %d\n", M_turned);
    if (M_served > 0)
    {
        printf("  Average response time: %.2f minutes\n", m_total_rt / M_served);
        printf("  Average turnaround time: %.2f minutes\n", m_total_tt / M_served);
        printf("  Throughput per seller: %.2f customers/hour\n", (double)M_served / 3.0); /// 60.0
    }
    printf("\n");

    // Low-price sellers
    printf("Low-Price Sellers (L1-L6):\n");
    printf("  Customers served: %d\n", L_served);
    printf("  Customers turned away: %d\n", L_turned);
    if (L_served > 0)
    {
        printf("  Average response time: %.2f minutes\n", l_total_rt / L_served);
        printf("  Average turnaround time: %.2f minutes\n", l_total_tt / L_served);
        // per hour
        printf("  Throughput per seller: %.2f customers/hour\n", (double)L_served / 6.0); /// 60.0
    }
    printf("\n");

    printf("Total served: %d\n", H_served + M_served + L_served);
    printf("Total turned away: %d\n", H_turned + M_turned + L_turned);
    printf("==========================================\n\n");
}

// Seller thread function
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// Each seller serves their own queue
void *sell(void *s_t)
{
    Seller *info = (Seller *)s_t;
    int myId = info->sellerID;
    char sellerType = info->sellerType;
    int myNumber = info->sellerNumber;
    char msg[200];

    int sellerFree = 0;

    // Serve customers in the queue
    while (nextCustomer[myId] < queueSizes[myId])
    {
        pthread_mutex_lock(&mutex);

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

        int seatRow = -1, seatCol = -1;
        int seatAssigned = 0;

        // Assign seat safely inside mutex
        if (availableSeats > 0)
        {
            seatAssigned = assignSeat(sellerType, customer->customerID, &seatRow, &seatCol);
        }

        if (seatAssigned)
        {
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

        nextCustomer[myId]++;
        pthread_mutex_unlock(&mutex);
    }

    return NULL;
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

    usleep(100000);

    for (currentTime = 0; currentTime <= 60; currentTime++)
    {
        pthread_cond_broadcast(&cond);

        usleep(50000);

        pthread_mutex_lock(&mutex);
        int seats = availableSeats;
        pthread_mutex_unlock(&mutex);

        if (seats == 0)
        {
            char msg[100];
            sprintf(msg, "Concert SOLD OUT at minute %d!", currentTime);
            printEvent(currentTime, msg);
            break;
        }
    }

    pthread_mutex_lock(&mutex);
    currentTime = 61;
    pthread_mutex_unlock(&mutex);
    pthread_cond_broadcast(&cond);

    usleep(100000); // Give threads time to exit

    printf("Waiting for all seller threads to exit...\n");
    for (int i = 0; i < 10; i++)
    {
        pthread_join(tids[i], NULL);
    }

    printSeatingChart(); // print final seating chart
    calculateStatistics();

    printf("Simulation complete.\n");

    return 0;
}
