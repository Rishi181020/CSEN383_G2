#ifndef SELLER_H
#define SELLER_H

typedef struct
{
    int sellerID;     // 0-9 (index)
    char sellerType;  // 'H', 'M', 'L'
    int sellerNumber; // 1 for H, 1-3 for M, 1-6 for L
} Seller;

#endif