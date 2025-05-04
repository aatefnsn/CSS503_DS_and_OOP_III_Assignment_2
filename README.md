# CSS503_DS_and_OOP_III_Assignment_2
CSS503 University of Washington Data structure and object oriented programming III Assignment 2
UWB Bothell
Professor Name: Morris Bernstein
Course: CSS-503


Assignment #2
Read me file



Student Name: Ahmed Nada






Assignment#2 description: 
Assignment #2 is simulating the very popular thread simulation sleeping barbers where barbers are constantly serving customers as they arrive at the shop else they will go to sleep until a customer wakes them up. If all barbers are busy and there is an empty waiting chair then the customer will sit down else the customer will leave without a service. 

The simulation is using a set of pre-defined public functions to access the cond variables of both the customers and the barbers to signal after each activity while the other counter part is waiting. Customer for instance has next_customer, finished and payment_accepted for the barber to call these functions to signal and hence stop the wait. Same for barbers, they have awaken, customer_sits, payment and closing time to be used by customers and shop to signal and stop the wait. 

Results:
Sample results shown below confirms that as the service time increase the number of turned away customers increases as well as the number of customers who had to wait to be serviced.
Same if we increase the customer arrivals. 
NBARBERS:3 NCHAIRS: 3 SERVICE_TIME: 1200 SERVICE_DEVIATION: 200 CUSTOMER_ARRIVALS: 400

customers served immediately: 568
customers waited 210
total customers served 778
customers turned away: 2
total customers: 780

NBARBERS:3 NCHAIRS: 3 SERVICE_TIME: 4000 SERVICE_DEVIATION: 200 CUSTOMER_ARRIVALS: 800

customers served immediately: 194
customers waited 192
total customers served 386
customers turned away: 0
total customers: 386

NBARBERS:3 NCHAIRS: 3 SERVICE_TIME: 8000 SERVICE_DEVIATION: 200 CUSTOMER_ARRIVALS: 600

customers served immediately: 110
customers waited 114
total customers served 224
customers turned away: 291
total customers: 515

NBARBERS:3 NCHAIRS: 3 SERVICE_TIME: 4000 SERVICE_DEVIATION: 200 CUSTOMER_ARRIVALS: 400

customers served immediately: 213
customers waited 213
total customers served 426
customers turned away: 353
total customers: 779

NBARBERS:3 NCHAIRS: 4 SERVICE_TIME: 4000 SERVICE_DEVIATION: 200 CUSTOMER_ARRIVALS: 400

customers served immediately: 213
customers waited 286
total customers served 499
customers turned away: 281
total customers: 780

Conclusion: 
While it appears that 3 Chairs is sufficient to not turn customers away, it depends on the barber service time and the customer arrivals time. 

Deliverables:

1-	Main.cpp containing my commented code
2-	Build.sh
3-	Run.sh
4-	Read me file
