#include <stdio.h>
#include <stdlib.h>
#include "customers.h"
#include <pthread.h>

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

// Seller thread function
#define MAX(a, b) ((a) > (b) ? (a) : (b))

// customer generation function for one seller
void generateCustomers(Customer queue[], int N, char sellerType, int sellerNumber)
{
    for (int i = 0; i < N; i++)
    {
        Customer *customer = &queue[i];

        // Set identity
        customer->sellerType = sellerType;
        customer->sellerID = sellerNumber;
        customer->customerNumber = i + 1;

        // Generate customer ID string
        sprintf(customer->customerID, "%c%d%02d", sellerType, sellerNumber, i + 1);

        // Random arrival time (0-59 minutes)
        customer->arrivalTime = rand() % 60;

        // Random service time based on seller type
        if (sellerType == 'H')
        {
            customer->serviceTime = 1 + (rand() % 2); // 1 or 2
        }
        else if (sellerType == 'M')
        {
            customer->serviceTime = 2 + (rand() % 3); // 2, 3, or 4
        }
        else
        {                                             // 'L'
            customer->serviceTime = 4 + (rand() % 4); // 4, 5, 6, or 7
        }

        // Initialize other fields
        customer->startTime = -1;
        customer->endTime = -1;
        customer->seatRow = -1;
        customer->seatCol = -1;
        customer->gotSeat = 0; // false
    }

    // Bubblesort customers by arrival time
    for (int i = 0; i < N - 1; i++)
    {
        for (int j = 0; j < N - i - 1; j++)
        {
            if (queue[j].arrivalTime > queue[j + 1].arrivalTime)
            {
                Customer temp = queue[j];
                queue[j] = queue[j + 1];
                queue[j + 1] = temp;
            }
        }
    }
}

// For debugging — prints all customers for one seller
void printQueue(Customer queue[], int N, char sellerType, int sellerNumber)
{
    printf("\n=== Seller %c%d Queue (N=%d) ===\n", sellerType, sellerNumber, N);
    printf("%-8s | Arrival | Service | Start | End | Seat\n", "ID");
    printf("---------|---------|---------|-------|-----|--------\n");

    for (int i = 0; i < N; i++)
    {
        Customer *c = &queue[i];
        printf("%-8s | %7d | %7d | %5d | %3d | (%d,%d)\n",
               c->customerID,
               c->arrivalTime,
               c->serviceTime,
               c->startTime,
               c->endTime,
               c->seatRow,
               c->seatCol);
    }
    printf("\n");
}

// Print events with timestamp
void printEvent(int time, char *message)
{
    int hours = time / 60;
    int minutes = time % 60;
    printf("[Time %02d:%02d] %s\n", hours, minutes, message);
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

// Function to calculate and print statistics
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
