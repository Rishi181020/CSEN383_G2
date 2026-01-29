#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "customers.h"

/*
10 ticket sellers to 100 seats concert during one hour. Each ticket
seller has their own queue for buyers.
*/
//#define MAX_CUSTOMERS_PER_SELLER 20 // or get N from command line



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

//customer generation function for one seller
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
        // Ex: "H101", "M205", "L304"

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

// Test function - prints all customers for one seller
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

void *sell(void *s_t)
{
    while (1) // having more work to do
    {
        char *sellerType = (char *)s_t; // get seller type
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond, &mutex); // wait until next buyer comes for this seller
        pthread_mutex_unlock(&mutex);
        // Serve any buyer available in this seller queue that is ready
        // now to buy ticket till done with all relevant buyers in their queue
    }
    return NULL; // thread exits
}

void wakeup_all_seller_threads()
{
    pthread_mutex_lock(&mutex);
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Usage: %s <number_of_customers>\n", argv[0]);
        return 1;
    }

    int N = atoi(argv[1]);
    srand(time(NULL)); // Don't forget this!

    Customer queues[10][20];
    int queueSizes[10];

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

    // TEST: Print out a few queues to verify
    printQueue(queues[0], N, 'H', 1); // H seller
    printQueue(queues[1], N, 'M', 1); // M1 seller
    printQueue(queues[4], N, 'L', 1); // L1 seller

    /*
    int i;
    pthread_t tids[10]; // thread ids for 10 seller threads
    char sellerType;
    int NUM_SELLERS = 10;

    // Create necessary data structures for the simulator.
    Customer queues[NUM_SELLERS][MAX_CUSTOMERS_PER_SELLER];
    int queueSizes[NUM_SELLERS]; // how many customers each has
    int nextCustomer[NUM_SELLERS];

    // Create buyers list for each seller ticket queue based on the
    // N value within an hour and have them in the seller queue.

    printf("Creating 10 threads representing the 10 sellers...\n");
    // Assume 10 threads, each represents a ticket seller: H1, M1, M2, M3, L1, L2, L3, L4, L5, L6.
    sellerType = 'H';
    pthread_create(&tids[0], NULL, sell, &sellerType);

    sellerType = 'M';
    for (i = 1; i < 4; i++)
        pthread_create(&tids[i], NULL, sell, &sellerType);
    sellerType = 'L';
    for (i = 4; i < 10; i++)
        pthread_create(&tids[i], NULL, sell, &sellerType);

    printf("Waking up all seller threads...\n");
    // wakeup all seller threads
    wakeup_all_seller_threads();

    printf("Waiting for all seller threads to exit...\n");
    // wait for all seller threads to exit
    for (i = 0; i < 10; i++)
        pthread_join(tids[i], NULL);

    // Printout simulation results

    printf("Simulation complete.\n");
    */

    return 0;
}
