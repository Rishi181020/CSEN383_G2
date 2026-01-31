
/*
Todo list in general:
- int assignSeat(char sellerType, int *seatRow, int *seatCol)
- void printSeatingChart()
- void printEvent(int time, char *message)
- void calculateStatistics()
- void *sell(void *s_t)
- void wakeup_all_seller_threads()
- main() function steps

*/


How to run:

First run this one to compile:
gcc -Wall -Wextra -g -std=c99 -o ticketSimulation main.c -lm

Then this one for executable:
./ticketSimulation <insert number of customers per seller to simulate>
  Ex: ./ticketSimulation 5
