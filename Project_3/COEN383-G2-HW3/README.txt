Goal: Build simulation written in C or C++ or Java programming language that
experiment with 10 ticket sellers to 100 seats concert during one hour. Each ticket
seller has their own queue for buyers.

For more details, please refer to the PDF file in this repo!
_________________________________________________________________________________________________


How to run:

(Go to directory where main.c is located)

First compile with:
gcc -pthread main.c -o ticketSimulation

Then this one with executable:
./ticketSimulation <insert number of customers per seller to simulate>
  Ex: ./ticketSimulation 5 > output5.txt
      ./ticketSimulation 10 > output10.txt
      ./ticketSimulation 15 > output15.txt
