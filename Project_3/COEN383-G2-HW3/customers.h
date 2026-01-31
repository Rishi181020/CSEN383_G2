#ifndef CUSTOMER_H
#define CUSTOMER_H

#include <stdbool.h>

typedef struct
{
    char customerID[10]; // "H001", "M205", "L304", etc.
    char sellerType;     // 'H', 'M', or 'L'
    int sellerID;        // 1-6 (which specific seller)
    int customerNumber;  // sequential number for this seller (1, 2, 3...)

    int arrivalTime; // when they join queue (0-59 minutes)
    int startTime;   // when seller starts serving them (-1 if never served)
    int serviceTime; // how long service takes (1-7 minutes, randomly assigned)
    int endTime;     // when they finish and leave (-1 if never served)

    int seatRow;  // assigned seat row (0-9), or -1 if no seat
    int seatCol;  // assigned seat column (0-9), or -1 if no seat
    bool gotSeat; // true if they got a seat, false if turned away

} Customer;

#endif