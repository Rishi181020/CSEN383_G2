#ifndef SELLER_H
#define SELLER_H

// A thread can find its own-id: thread-id = pthread_self();
// How a thread can find out about its own customers’ queue?
// option-3: when creating the thread, pass a struct that includes the index “i” and seller_type
// seller thread to serve one time slice (1 minute)

typedef struct
{
    int sellerID;     // 0-9 (index)
    char sellerType;  // 'H', 'M', 'L'
    int sellerNumber; // 1 for H, 1-3 for M, 1-6 for L
} Seller;

#endif