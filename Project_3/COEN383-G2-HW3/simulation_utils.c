#include <stdio.h>
#include <stdlib.h>
#include "customers.h"
#include <pthread.h>

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
