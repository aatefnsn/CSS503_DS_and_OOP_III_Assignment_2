//
// barbers.cc
//
// Simulate the sleeping barbers problem.
//
// Author: Morris Bernstein
// Copyright 2019, Systems Deployment, LLC.

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <pthread.h>
#include <queue>
#include <random>
#include <time.h>
#include <unistd.h>
#include <vector>
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<errno.h>
#include<sys/ipc.h>
#include <queue>
#include <iostream>
using namespace std;

enum barber_status { // enums as i was planning to use them to control the barber statuses but i ended up using flags instead
    asleep, awake, busy, request, collect, barberleave
};

enum customer_status { // enums but not used
    waiting, standup, HavingHairCut, paying, leave, leavenoservice, DONEWITHHAIRCUT
};

const char* PROG_NAME = "";

class Shop;
class Barber;
class Customer;

class Lock {
public:

    Lock(pthread_mutex_t* mutex) : mutex(mutex) {
        int rc = pthread_mutex_lock(this->mutex);
        if (rc < 0) {
            perror("can't lock mutex");
            exit(EXIT_FAILURE);
        }
    }

    ~Lock() {
        int rc = pthread_mutex_unlock(mutex);
        if (rc < 0) {
            perror("can't unlock mutex");
            exit(EXIT_FAILURE);
        }
    }

private:
    pthread_mutex_t* mutex;
};

class Shop {
public:
    bool shop_open = true;

    struct BarberOrWait {
        Barber* barber; // Barber is available
        bool chair_available; // If barber isn't available,
        // waiting chair is available.
    };

    // Constructor initializes shop and creates Barber threads (which
    // will immediately start calling next_customer to fill the
    // collection of sleeping barbers).
    Shop(int n_barbers,
            unsigned int waiting_chairs,
            int average_service_time,
            int service_time_deviation,
            int average_customer_arrival,
            int duration);

    // Main thread: open the shop and spawn customer threads until
    // closing time.  Report summary statistics for the day.
    void run();

    // Customer thread announces arrival to shop. If the collection of
    // currently sleeping barbers is not empty, remove and return one
    // barber from the collection. If all the barbers are busy and there
    // is an empty chair in the waiting room, add the customer to the
    // waiting queue and return {nullptr, true}.  Otherwise, the
    // customer will leave: return {nullptr, false}.
    BarberOrWait arrives(Customer* customer);

    // Barber thread requests next customer.  If no customers are
    // currently waiting, add the barber to the collection of
    // currently sleeping barbers and return nullptr.
    Customer* next_customer(Barber* barber);

    // Return random service time
    int service_time();

    // Return random customer arrival.
    int customer_arrival_time();
    //pthread_cond_t * cond_barber; //array
    //pthread_cond_t * cond_customer;

    pthread_mutex_t* shop_mutex;
    pthread_mutex_t* nextcustomer_mutex;

private:
    vector<pthread_t*> barber_threads;
    struct timespec time_limit;
    int customers_served_immediately;
    int customers_waited;
    int customers_turned_away=0;
    int customers_total;
    queue<Customer*> chairs;
    queue<Barber*> sleeping_barbers;
    int average_customer_arrival;
    int service_time_deviation;
    int average_service_time;
    int n_barbers;
    //Barber* barbers_array;
    unsigned int waiting_chairs; // should be unsigned 
    queue <pthread_t*> customer_thread_queue;
    void close();
    void cleanup();
};

class Barber {
public:
    bool gohome; // boolean to mark if it is time for the barber to go home after being sleeping
    bool givinghaircut; // boolean to control the barber status
    bool hassitting; // boolean to control barber status, has a customer sitting on the chair so he should start serving
    bool gotpaid; // boolean to control barber status, whether he got paid or not

    Barber(Shop* shop, int id);

    // Barber thread function.
    void run();

    // Shop tells barber it's closing time.
    void closing_time();

    // Customer wakes up barber.
    void awaken(Customer* customer);

    // Customer sits in barber's chair.
    void customer_sits();

    // Customer proffers payment.
    void payment();

    /*const*/ int id = 0;

private:
    Customer* myCustomer; // a barber has a customer
    pthread_cond_t* cond_barber; // cond variable for the barber
    pthread_mutex_t* barber_mutex; // barber mutex
    barber_status barber_state; // each barber has a state from the enums but it is not used
    Shop* shop;

    Barber(const Barber&) = delete;
};

class Customer {
public:
    bool awakenedbarber; // a boolean to control Customer states, customer awakened the barber
    bool hadhaircut; // a boolean to control if a customer has had his haircut
    bool paid; // a boolean to control if the customer paid the barber or not
    
    Customer();
    Customer(Shop* shop, int id);
    ~Customer();

    void run();

    // Barber calls this customer after checking the waiting-room
    // queue (customer should be waitint).
    void next_customer(Barber* barber);

    // Barber finishes haircut (Customer should be receiving haircut).
    void finished();

    // Barber has accepted payment for service.
    void payment_accepted();

    /*const*/ int id /*= 0*/;

private:
    Shop* shop;
    Barber* myBarber; // a customer has a barber to serve him
    pthread_cond_t* cond_customer; // conditional variable for the customer
    pthread_mutex_t* customer_mutex; // mutex for the customer
    customer_status customer_state; // customer enum
    // Customer thread function.

    Customer(const Customer&) = delete; // to prevent calling copy constructor

};

Barber::Barber(Shop* shop, int id) {
    this->gotpaid=false; // setting all conditions ti false
    this->givinghaircut=false;
    this->hassitting=false;
    this->gohome = false;
    this->id = id;
    this->shop = shop;
    this->myCustomer = nullptr;
    this->cond_barber = reinterpret_cast<pthread_cond_t*> (malloc(sizeof (pthread_cond_t))); // allocting mem for cond var
    pthread_cond_init(this->cond_barber, NULL); // initializing the cond variable
    this->barber_mutex = reinterpret_cast<pthread_mutex_t*> (malloc(sizeof (pthread_mutex_t))); // allocating mem for mutex
    pthread_mutex_init(barber_mutex, NULL); // intializing the mutex
}

// Barber thread.  Must be joined by Shop thread to allow shop to
// determine when barbers have left.

void* run_barber(void* arg) {
    Barber* barber = reinterpret_cast<Barber*> (arg);
    barber->run();
    return nullptr;
}

void Barber::closing_time() { // function to turn the boolean of go home to true so that the barber knows the shop has closed
    pthread_mutex_lock(this->barber_mutex);
    this->gohome = true;
    pthread_cond_signal(this->cond_barber);
    pthread_mutex_unlock(this->barber_mutex);
}

void Barber::awaken(Customer* customer) { // function to assign a customer to the barber
    pthread_mutex_lock(this->barber_mutex);
    this->myCustomer = customer;
    pthread_cond_signal(this->cond_barber);
    pthread_mutex_unlock(this->barber_mutex);
}

void Barber::customer_sits() { // function to let the barber know it has a customer sitting 
    pthread_mutex_lock(this->barber_mutex);
    this->hassitting = true;
    pthread_cond_signal(cond_barber);
    pthread_mutex_unlock(this->barber_mutex);
}

void Barber::payment() { // function to let the barber know if the customer has paid him or not
    pthread_mutex_lock(this->barber_mutex);
    this->gotpaid = true;
    pthread_cond_signal(cond_barber);
    pthread_mutex_unlock(this->barber_mutex);
}

Customer::Customer(Shop* shop, int id) {
    this->paid=false;    // setting all flags to false
    this->awakenedbarber=false;
    this->hadhaircut=false;
    this->shop = shop;
    this->id = id;
    this->myBarber = nullptr; // initializing the barber to be nullptr
    this->cond_customer = reinterpret_cast<pthread_cond_t*> (malloc(sizeof (pthread_cond_t))); //allocating mem to cond var
    pthread_cond_init(this->cond_customer, NULL); // initializing it
    this->customer_mutex = reinterpret_cast<pthread_mutex_t*> (malloc(sizeof (pthread_mutex_t)));// allocating mem to mutex
    pthread_mutex_init(customer_mutex, NULL); // initalizing mutex
}

// Customer thread.  Runs in detatched mode so resources are
// automagically cleaned up when customer leaves shop.

void* run_customer(void* arg) {
    Customer* customer = reinterpret_cast<Customer*> (arg);
    customer->run();
    delete customer;
    return nullptr;
}

Customer::~Customer() {
    cout << "Calling Customer destructor: Deleting customer" << endl;
    pthread_cond_destroy(this->cond_customer); // destroying yhe cond variable so no mem leaks
    pthread_mutex_destroy(this->customer_mutex); // destroying the mutex to free mem
}

void Customer::run() { // customer thread
    cout << "Customer " << this->id << " arrived at the shop" << endl;
    bool chair;
    Barber* wakedup;
    Shop::BarberOrWait b = this->shop->arrives(this);
    wakedup = b.barber;
    chair = b.chair_available;

    if (wakedup == nullptr) { // check if there is no barber available
        if (chair == true) { // if there is a chair then the customer will wait in the waiting room
            cout << "Customer " << this->id << " takes a seat in the waiting room" << endl;
            pthread_mutex_lock(this->customer_mutex);
            while (this->myBarber == nullptr) { // customer will wait for a barber to call him
                pthread_cond_wait(this->cond_customer, this->customer_mutex);
            }
            this->customer_state = standup;
            pthread_mutex_unlock(this->customer_mutex);
        } else {
            cout << "Customer " << this->id << " leaves without getting a hair cut" << endl;
	   return; // if the customer has no chairs in the waiting room then it will just leave 
        }
    } else {
        pthread_mutex_lock(this->customer_mutex);
        this->myBarber = wakedup; // if the customer has a barber then assign its barber to the waked up barber
        this->customer_state = standup;
        pthread_mutex_unlock(this->customer_mutex);
    }

    pthread_mutex_lock(this->customer_mutex);
    cout << "Customer " << this->id << " wakes barber " << this->myBarber->id << endl; // waking up barber
    this->myBarber->awaken(this); // calling awaken for the customer to awaken the barber assigned to him
    while (this->awakenedbarber == false) {
        pthread_cond_wait(this->cond_customer, this->customer_mutex); // after awaking the barber, customer will wait then sit
        cout << "Customer " << this->id << " sits in barber's " << this->myBarber->id << " chair" << endl;
    }
    pthread_mutex_unlock(this->customer_mutex);

    this->myBarber->customer_sits();

    pthread_mutex_lock(this->customer_mutex);
    while (this->hadhaircut == false) { // wait until barber finishes the hair cut
        pthread_cond_wait(this->cond_customer, this->customer_mutex);
        cout << "Customer " << this->id << " gets up and proffers payment to barber " << this->myBarber->id << endl; //cutomer g       //gets up and offer to pay
    }
    pthread_mutex_unlock(this->customer_mutex);

    this->myBarber->payment(); // call the payment method to signal

    pthread_mutex_lock(this->customer_mutex);
    while (this->paid == false) {
        pthread_cond_wait(this->cond_customer, this->customer_mutex); // wait until the barber accepts the payment
        this->myBarber=nullptr;
        cout << "Customer " << this->id << " leaves statisfied" << endl; // customer leaves
    }
    pthread_mutex_unlock(this->customer_mutex);
}

// Barber calls this customer after checking the waiting-room
// queue (customer should be waitint).

void Customer::next_customer(Barber* barber) {
    pthread_mutex_lock(this->customer_mutex);
    this->myBarber = barber; // assign the barber to the customer
    this->awakenedbarber=true;
    pthread_cond_signal(this->cond_customer);
    pthread_mutex_unlock(this->customer_mutex);
}

void Customer::finished() { // function to set that the customer finished his hair cut and signal
    pthread_mutex_lock(this->customer_mutex);
    this->hadhaircut = true;
    pthread_cond_signal(this->cond_customer);
    pthread_mutex_unlock(this->customer_mutex);
}

void Customer::payment_accepted() { // function to set customer to paid
    pthread_mutex_lock(this->customer_mutex);
    this->paid=true;
    pthread_cond_signal(this->cond_customer);
    pthread_mutex_unlock(this->customer_mutex);
}

void Barber::run() {
    cout << "Barber " << this->id << " arrives for work" << endl;
    Customer * nextcustomer;
    while (true) { // barber thread goes into an infinite while loop until it gets broken by shop closing
        nextcustomer = this->shop->next_customer(this); // calls next customer to check the waiting room
        if (nextcustomer != nullptr) { // if there is a customer waiting then call him
            cout << "Barber " << this->id << " calls customer " << this->myCustomer->id << endl;
            nextcustomer->next_customer(this);
        }
        else if (nextcustomer == nullptr && !this->shop->shop_open) { // if there are no customers and the shop is closed then b          //arber leaves for home
            cout << "No more customers and shop is closed, barber " << this->id << " leaves for home" << endl;
            break;
        }
        else { // if there is no csutmers waiting but shop is still open then go for a nap
            pthread_mutex_lock(this->barber_mutex);
            cout << "Barber " << this->id << " goes for a nap" << endl;
            // did the shop close?
            while (this->myCustomer == nullptr && !this->gohome) {
                pthread_cond_wait(this->cond_barber, this->barber_mutex); //wait until a customer arrives to wake the barber up
            }

            if (this->gohome) {
                cout << "Wake up barber " << this->id << " !!! please go home" << endl; // if while sleeping the shop has closed //then break the loop and go home
                break;
            }
            // if no customer, shop must be closed...
            this->barber_state = awake; 
            nextcustomer = this->myCustomer; // set the next customer to myCustomer
            pthread_mutex_unlock(this->barber_mutex);
        }

        // barber services customer
        // usleep...
        // customer->finished()
        // wait_for_payment()
        //      check (mutex & condition_wait (if necessary)) 
        //           this->paid or this->state == paid
        // customer->accepted_payement()
        //
        // reset state variables 

        cout << "Barber " << this->id << " wakes up" << endl;
        this->myCustomer->next_customer(this); // barber wakes up and sets the next customer to his customer

        pthread_mutex_lock(this->barber_mutex);
        while (this->hassitting == false) { // wait until the customer sits down
            pthread_cond_wait(this->cond_barber, this->barber_mutex);
            cout << "Barber " << this->id << " gives customer " << this->myCustomer->id << " a haircut " << endl;
            usleep(this->shop->service_time()*1000); // service time
            cout << "Barber " << this->id << " finishes customer " << this->myCustomer->id << "'s haircut " << endl; // finish t//he hair cut
        }
        pthread_mutex_unlock(this->barber_mutex);

        nextcustomer->finished(); // call finished to signal to customer

        pthread_mutex_lock(this->barber_mutex);
        while (this->gotpaid == false) { // wait until the customer pays
            pthread_cond_wait(this->cond_barber, this->barber_mutex);
            cout << "Barber " << this->id << " accepts payment from customer " << this->myCustomer->id << endl;
        }
        pthread_mutex_unlock(this->barber_mutex);

        nextcustomer->payment_accepted(); // call payment accepted to signal to customer

        pthread_mutex_lock(this->barber_mutex);
        this->hassitting = false; // reseting barber states
        this->gotpaid = false; //resetting states
        this->myCustomer = nullptr; // unassigning the current customer as he is done with his haircut
        pthread_mutex_unlock(this->barber_mutex);
    }
}

// Constructor initializes shop and creates Barber threads (which
// will immediately start calling next_customer to fill the
// collection of sleeping barbers).

Shop::Shop(int n_barbers,
        unsigned int waiting_chairs,
        int average_service_time,
        int service_time_deviation,
        int average_customer_arrival,
        int duration) {

    int rc = clock_gettime(CLOCK_REALTIME, &time_limit);
    if (rc < 0) {
        perror("reading realtime clock");
        exit(EXIT_FAILURE);
    }
    // Round to nearest second
    time_limit.tv_sec += duration + (time_limit.tv_nsec >= 500000000);
    time_limit.tv_nsec = 0;

    // Initializing mutex

    this->shop_mutex = reinterpret_cast<pthread_mutex_t*> (malloc(sizeof (pthread_mutex_t))); // memory allocation for shp mutex
    pthread_mutex_init(this->shop_mutex, NULL); // initializing shop mutex
    this->n_barbers = n_barbers; // setting number of barbers
    // Creating Barber Threads

    cout << "Creating Barber Threads " << n_barbers << " barbers" << endl;
    for (int i = 0; i< this->n_barbers; i++) {
        Barber * barber = new Barber(this, i); //id = n_barbers and keeps incrementing 0 -> n_barbers

        pthread_t* thread = reinterpret_cast<pthread_t*> (calloc(1, sizeof (pthread_t))); // mem allocation for thread
        int rc = pthread_create(thread, nullptr, run_barber, (void*) barber); // thread creation using the created barber
        if (rc != 0) {
            errno = rc;
            perror("creating pthread");
            exit(EXIT_FAILURE);
        }
    }

    this->waiting_chairs = waiting_chairs; //setting shop variables
    this->average_customer_arrival = average_customer_arrival;
    this->service_time_deviation = service_time_deviation;
    this->average_service_time = average_service_time;
}

void Shop::run() {
    //cout << "the Barber shop opens" << endl;
    if (!shop_open) { // initilizing barber shop
        this->customers_served_immediately = 0;
        this->customers_waited = 0;
        this->customers_turned_away = 0;
        this->customers_total = 0;
        this->shop_open = true;
        cout << "the Barber shop opens" << endl;
    }

    for (int next_customer_id = 0;; next_customer_id++) {
        struct timespec now;
        clock_gettime(CLOCK_REALTIME, &now);
        if (now.tv_sec >= time_limit.tv_sec) {
            // Shop closes.
            break;
        }// Wait for random delay, then create new Customer thread.
        else {
            pthread_t* thread = reinterpret_cast<pthread_t*> (calloc(1, sizeof (pthread_t))); // mem alloc for customer thread
            Customer * customer = new Customer(this, next_customer_id); // creating customer
            this->customers_total++; // incrementing number of customers
            int rc2 = pthread_create(thread, nullptr, run_customer, reinterpret_cast<void *> (customer)); // creating thread
            pthread_detach(*thread); // detach thread
            if (rc2 != 0) {
                errno = rc2;
                perror("creating pthread");
                exit(EXIT_FAILURE);
            }
            int sleep_value_ms = this->customer_arrival_time();
            usleep(sleep_value_ms * 1000); // sleep inbetween customer creations
        }
    }
    this->shop_open = false; // closing the shop
    
    while (this->sleeping_barbers.size() > 0){
        Barber * toTerminate;
        toTerminate=sleeping_barbers.front();
        sleeping_barbers.pop();
        toTerminate->closing_time(); // calling closeing time to set the gohome bool flag of barbers so if they are sleeping and //shop has closed, then closing time will signal them to wake up and go home
    }
            
    cout << "the shop closes" << endl;
    for (auto thread : barber_threads) {
        pthread_join(*thread, nullptr);
    }
    cout << "customers served immediately: " << customers_served_immediately << endl;
    cout << "customers waited " << customers_waited << endl;
    cout << "total customers served " << (customers_served_immediately + customers_waited) << endl;
    cout << "customers turned away: " << customers_turned_away << endl;
    cout << "total customers: " << customers_total << endl;
}

// Customer thread announces arrival to shop. If the collection of
// currently sleeping barbers is not empty, remove and return one
// barber from the collection. If all the barbers are busy and there
// is an empty chair in the waiting room, add the customer to the
// waiting queue and return {nullptr, true}.  Otherwise, the
// customer will leave: return {nullptr, false}.

Shop::BarberOrWait Shop::arrives(Customer* customer) {
    // Find a sleeping barber.
    // No barber: check for a waiting-area chair.
    // Otherwise, customer leaves.
    BarberOrWait * b= new BarberOrWait(); // fake barber or wait for return purposes to avoid the warning
    Barber* wakedup_barber; // waked up barber

    pthread_mutex_lock(this->shop_mutex);
    if (this->sleeping_barbers.empty()) { // if no sleeping barbers
        if ((unsigned int)this->chairs.size() <= (this->waiting_chairs) - 1) { // check if there is at least one waiting chair
            this->chairs.push(customer); // if there is a chair .. push the customer
            this->customers_waited++; // increment the counter of customer waited
            pthread_mutex_unlock(shop_mutex);
            return {nullptr, true};
        } else if ((unsigned int)this->chairs.size() == this->waiting_chairs) { // if no chairs
            this->customers_turned_away++; // increment the number of turned away
            pthread_mutex_unlock(shop_mutex);
            return {nullptr, false};
        }
    } else {
        wakedup_barber = this->sleeping_barbers.front(); // if there is a sleeping barber
        this->sleeping_barbers.pop(); // pop one barber
        this->customers_served_immediately++; // increment the served immediately
        pthread_mutex_unlock(shop_mutex);
        return {wakedup_barber, true};
    }
    return *b;
}

// Barber thread requests next customer.  If no customers are
// currently waiting, add the barber to the collection of
// currently sleeping barbers and return nullptr.

Customer* Shop::next_customer(Barber* barber) {
    Customer * nextcustomer;
    pthread_mutex_lock(shop_mutex);
    if (!this->chairs.empty()) {
        nextcustomer = chairs.front(); // assign the next customer
        chairs.pop(); // empty the chairs queue by one 
        barber->awaken(nextcustomer); // assign the barber to the customer by calling awaken
        pthread_mutex_unlock(shop_mutex);
        return nextcustomer;
    } else {
        sleeping_barbers.push(barber); // if no waiting customers push the barber into sleeping barbers queue
        pthread_mutex_unlock(shop_mutex); 
        return nullptr;
    }
}

int Shop::service_time() { // function to return the sevice time as requested
    int number;
    std::default_random_engine generator;
    std::normal_distribution<double> distribution(this->average_service_time, this->service_time_deviation); 
    do {
        number = distribution(generator);
    } while (number < 0.8 * this->average_service_time); // make sure to clip service time by 80% of the barber average ser time
    return number;
}

int Shop::customer_arrival_time() { // return the customer arrival time using poisson dist
    std::default_random_engine generator;
    std::poisson_distribution<int> distribution(this->average_customer_arrival);
    int number = distribution(generator);
    return number;
}

void usage() {
    cerr
            << "usage: "
            << PROG_NAME
            << " <nbarbers>"
            << " <nchairs>"
            << " <avg_service_time>"
            << " <service_time_std_deviation>"
            << " <avg_customer_arrival_time>"
            << " <duration>"
            << endl;
    exit(EXIT_FAILURE);
}

int main(int argc, char* argv[]) {
    PROG_NAME = argv[0];

    if (argc != 7) {
        usage();
    }
    int barbers = atoi(argv[1]);
    if (barbers <= 0) {
        usage();
    }
    int chairs = atoi(argv[2]);
    if (chairs < 0) {
        usage();
    }
    int service_time = atoi(argv[3]);
    if (service_time <= 0) {
        usage();
    }
    int service_deviation = atoi(argv[4]);
    if (service_time <= 0) {
        usage();
    }
    int customer_arrivals = atoi(argv[5]);
    if (customer_arrivals <= 0) {
        usage();
    }
    int duration = atoi(argv[6]);
    if (duration <= 0) {
        usage();
    }

    Shop barber_shop(barbers,
            chairs,
            service_time,
            service_deviation,
            customer_arrivals,
            duration);
    barber_shop.run();

    return EXIT_SUCCESS;
}
