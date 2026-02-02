Goal: Build simulation written in C or C++ or Java programming language that
experiment with 10 ticket sellers to 100 seats concert during one hour. Each ticket
seller has their own queue for buyers.


Todo list:
o Calculate average RT, WT, and TT per customer for a given seller type.
o Function to print out statistics for each seller type: void calculateStatistics()
o void *sell(void *s_t)
o Flesh out main() 
o Add event printing using printSeatingChart() on line 109 and printEvent() on line 148.
X int assignSeat(char sellerType, int *seatRow, int *seatCol)
    - Assign seat based on seller type (H, M, L)
    - Ensure no double booking of seats. (Mutex needed)


o Ensure correct syntax for output file: (just use void printEvent(int time, char *message) on line 148 )
    "Your program should print a line indicating each event as it occurs. 
      An event is:
        • A customer arrives at the tail of a seller’s queue.
        • A customer is served and is assigned a seat, or is told the concert is sold out,
          in which case the customer immediately leaves.
        • A customer completes a ticket purchase and leaves.
        • In addition, your program should compute the Average response time per
          customer for a given seller type, average turn-around time per customer
          for a given seller type, average throughput per seller type
          Start each event print line with a time stamp, such as 0:05, 0:12, etc."

    (Use helper function printSeatingChart() on line 109))
        "After each ticket sale, also print the concert seating chart as a 10-by-10 matrix that
        shows which customer was assigned to each seat. Identify ticket seller H’s cus-
        tomers as H001, H002, H003, ...; the customers of ticket sellers M1, M2, …, as
        M101, M102, …, M201, M202, …; and the customers of ticket sellers L1, L2, …
        as L101, L102, …, L201, L202, ... You can indicate still-unsold seats with dashes..."


"At the end of one hour, each ticket seller should complete whatever purchase may
still be in progress and close the ticket window. Any remaining customers in the
queues should leave immediately. Of course, if the concert sells out before the hour
is up, all the ticket windows should close after the last seat is taken..."

Run your program for N = 5, 10, and 15 customers per ticket seller, where N is
a command-line parameter.

At the end of each run, print how many H, M, and L customers got seats, how
many customers were turned away, etc.

_________________________________________________________________________________________________


How to run:

First run this one to compile:
gcc -pthread main.c -o ticketSimulation


Then this one for executable:
./ticketSimulation <insert number of customers per seller to simulate>
  Ex: ./ticketSimulation 5 > output_N5.txt
      ./ticketSimulation 10 > output_N10.txt
      ./ticketSimulation 15 > output_N15.txt
