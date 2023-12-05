#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <semaphore.h>
#include <limits.h>

#define MAX_TOPPING_LENGTH 20
#define MAX_FLAVOUR_LENGTH 20
#define MAX_STRING_LENGTH 100
#define MAX_INTEGER_ARRAY_LENGTH 100

#define WHITE "\x1b[37m"
#define YELLOW "\x1b[33m"
#define CYAN "\x1b[36m"
#define BLUE "\x1b[34m"
#define GREEN "\x1b[32m"
#define RED "\x1b[31m"
#define ORANGE "\x1b[38;5;208m"
#define RESET "\x1b[0m"

int init_time;
int get_current_time()
{
    return time(NULL) - init_time;
}

typedef struct node_t node_t;
struct node_t
{
    int id;
    node_t *next;
};
typedef node_t *node_ptr;

typedef struct machine_t machine_t;
struct machine_t
{
    int machine_id;
    int start_time;
    int end_time;
};

typedef struct flavour_t flavour_t;
struct flavour_t
{
    int flavour_id;
    char *flavour_name;
    int preparation_time;
};

typedef struct topping_t topping_t;
struct topping_t
{
    int topping_id;
    char *topping_name;
    int quantity;
};

typedef struct order_t order_t;
struct order_t
{
    int order_id;
    int customer_id;
    node_ptr icecream;
    int order_completed;
};

typedef struct customer_t customer_t;
struct customer_t
{
    int customer_id;
    int arrival_time;
    int number_of_icecreams;
    node_ptr *icecreams;
};

int N, K, F, T;
machine_t **list_of_machines;
flavour_t **list_of_flavours;
topping_t **list_of_toppings;

int customers_in_line;
pthread_mutex_t lock_customers_in_line = PTHREAD_MUTEX_INITIALIZER; // LOCK USED WHENEVER WE MAKE CHANGES TO THE customers_in_line VARIABLE
int store_closing_time;                                             // THE TIME WHEN THE LAST MACHINE CLOSES

pthread_t store_thread;
sem_t machines_semaphore;
sem_t *list_of_semaphores_for_machines;
int *machines_working; // ARRAY WHICH HOLD INFORMATION ABOUT WHICH MACHINE IS RUNNING
// 0 -> MACHINE NOT STARTED
// 1 -> MACHINE STARTED AND IS EMPTY
// 2 -> MACHINE STARTED AND IS OCCUPIED
// -1 -> MACHINE STOPPED

pthread_mutex_t lock_ingredients_availability = PTHREAD_MUTEX_INITIALIZER; // LOCK USED WHEN WE WANT TO CHECK THE AVAILABILITY OF TOPPINGS

customer_t *create_customer(int customer_id, int arrival_time, int number_of_icecreams, node_ptr *icecreams)
{
    customer_t *ret = (customer_t *)calloc(1, sizeof(customer_t));
    ret->customer_id = customer_id;
    ret->arrival_time = arrival_time;
    ret->number_of_icecreams = number_of_icecreams;
    ret->icecreams = icecreams;
    return ret;
}

order_t *create_order(int order_id, int customer_id, node_ptr icecream)
{
    order_t *ret = (order_t *)calloc(1, sizeof(order_t));
    ret->order_id = order_id;
    ret->customer_id = customer_id;
    ret->icecream = icecream;
    ret->order_completed = 0;
    return ret;
}
node_ptr create_node(int id)
{
    node_ptr ret = (node_ptr)malloc(sizeof(node_t));
    ret->id = id;
    ret->next = NULL;
    return ret;
}

void insert_node(node_ptr linked_list, int id)
{
    node_ptr curr = linked_list;
    while (curr->next != NULL)
    {
        curr = curr->next;
    }
    node_ptr new_node = create_node(id);
    curr->next = new_node;
}

int get_flavour_id(char *flavour_name)
{
    for (int i = 0; i < F; i++)
    {
        if (strcmp(list_of_flavours[i]->flavour_name, flavour_name) == 0)
        {
            return list_of_flavours[i]->flavour_id;
        }
    }
    return -1;
}

int get_flavour_preparation_time(int flavour_id)
{
    for (int i = 0; i < F; i++)
    {
        if (flavour_id == list_of_flavours[i]->flavour_id)
        {
            return list_of_flavours[i]->preparation_time;
        }
    }
    return -1;
}

int get_topping_id(char *topping_name)
{
    for (int i = 0; i < T; i++)
    {
        if (strcmp(list_of_toppings[i]->topping_name, topping_name) == 0)
        {
            return list_of_toppings[i]->topping_id;
        }
    }
    return -1;
}

int get_topping_quantity(int topping_id)
{
    for (int i = 0; i < T; i++)
    {
        if (topping_id == list_of_toppings[i]->topping_id)
        {
            return list_of_toppings[i]->quantity;
        }
    }
    return INT_MIN;
}

void use_topping(int topping_id)
{
    for (int i = 0; i < T; i++)
    {
        if (topping_id == list_of_toppings[i]->topping_id)
        {
            list_of_toppings[i]->quantity--;
            return;
        }
    }
}

// IF ORDER CAN BE FULFILLED, THE FUNCTION DECREMENTS THE QUANTITIES OF THE TOPPINGS WE ARE GOING TO USE
int can_fulfill_order(int number_of_icecreams, node_ptr *icecreams)
{
    pthread_mutex_lock(&lock_ingredients_availability);
    for (int i = 0; i < number_of_icecreams; i++)
    {
        node_ptr icecream = icecreams[i];
        node_ptr topping = icecream->next;
        while (topping != NULL)
        {
            int topping_id = topping->id;
            int quantity = get_topping_quantity(topping_id);
            assert(quantity != INT_MIN); // ERROR HANDLING

            if (!quantity) // IF TOPPING QUANTITY IS 0, RETURN 0
            {
                pthread_mutex_unlock(&lock_ingredients_availability);
                return 0;
            }
            topping = topping->next;
        }
    }
    // ALL THE TOPPINGS ARE AVAILABLE
    for (int i = 0; i < number_of_icecreams; i++)
    {
        node_ptr icecream = icecreams[i];
        node_ptr topping = icecream->next;
        while (topping != NULL)
        {
            use_topping(topping->id);
            topping = topping->next;
        }
    }
    pthread_mutex_unlock(&lock_ingredients_availability);
    return 1;
}

int customer_leaves_with_unfulfilled_order_handler()
{
    // THE NUMBER OF CUSTOMERS IN THE LINE DECREASES AFTER 1 UNIT TIME
    sleep(1);

    // DECREMENT THE NUMBER OF CUSTOMERS IN LINE
    pthread_mutex_lock(&lock_customers_in_line);
    customers_in_line--;
    pthread_mutex_unlock(&lock_customers_in_line);
}

int check_suitable_machine(int i, int preparation_time)
{
    // MACHINE HAS NOT STARTED WORKING OR MACHINE IS CURRENTLY OCCUPIED
    if (machines_working[i] != 1)
        return 0;

    // CALCULATE THE TIME LEFT FOR THE MACHINE TO RUN
    int end_time = list_of_machines[i]->end_time;
    int machine_running_time_left = end_time - get_current_time();

    // MACHINE HAS ENOUGH TIME TO PREPARE THE ICECREAM
    if (machine_running_time_left >= preparation_time)
    {
        return 1;
    }
    return 0;
}
// FUNCTIONS TO PRINT THE INPUTS AND CHECK IF INPUT IS BEING TAKEN IN CORRECTLY

void printMachines()
{
    for (int i = 0; i < N; i++)
    {
        printf("Machine ID: %d\n", list_of_machines[i]->machine_id);
        printf("Start Time: %d\n", list_of_machines[i]->start_time);
        printf("End Time: %d\n", list_of_machines[i]->end_time);
        printf("\n");
    }
}

void printFlavours()
{
    for (int i = 0; i < F; i++)
    {
        printf("Flavour ID: %d\n", list_of_flavours[i]->flavour_id);
        printf("Flavour Name: %s\n", list_of_flavours[i]->flavour_name);
        printf("Preparation Time: %d\n", list_of_flavours[i]->preparation_time);
        printf("\n");
    }
}

void printToppings()
{
    for (int i = 0; i < T; i++)
    {
        printf("Topping ID: %d\n", list_of_toppings[i]->topping_id);
        printf("Topping Name: %s\n", list_of_toppings[i]->topping_name);
        printf("Quantity: %d\n", list_of_toppings[i]->quantity);
        printf("\n");
    }
}

// CHECK FUNCTIONS COMPLETED

// THREAD FUNCTIONS

void *store_thread_function(void *args)
{

    while (1)
    {
        for (int i = 0; i < N; i++)
        {
            machine_t *current_machine = list_of_machines[i];

            int id = current_machine->machine_id;
            int start_time = current_machine->start_time;
            int end_time = current_machine->end_time;
            int time_now = get_current_time();

            // CHECK IF MACHINE HAS STARTED WORKING
            if (!machines_working[i] && start_time <= time_now)
            {
                // INCREMENT THE NUMBER OF MACHINES IN THE STORE BY 1
                sem_post(&machines_semaphore);
                printf(ORANGE "Machine %d has started working at %d second(s)\n" RESET, id, time_now);
                machines_working[i] = 1;
            }
            // CHECK IF MACHINE HAS FINISHED WORKING
            if (machines_working[i] != -1 && end_time <= time_now)
            {
                // DECREMENT THE NUMBER OF MACHINES IN THE STORE BY 1
                sem_wait(&machines_semaphore);
                printf(ORANGE "Machine %d has stopped working at %d second(s)\n" RESET, id, time_now);
                machines_working[i] = -1;
            }
        }
        // CHECK IF STORE CLOSING TIME IS HERE
        if (get_current_time() > store_closing_time)
        {
            sleep(1);
            printf("Parlour Closed\n");
            pthread_exit(NULL);
        }
    }
}

void *machine_thread_function(void *args)
{
    order_t *order = (order_t *)args;
    int flavour_id = order->icecream->id;
    int order_id = order->order_id;
    int customer_id = order->customer_id;

    int machine_id = -1;
    int preparation_time = get_flavour_preparation_time(flavour_id);
    assert(preparation_time != -1); // ERROR HANDLING

    int machine_allocated = 0;

    sleep(1);
    // LOOP FOR FINDING A MACHINE
    while (1)
    {
        // STORE IS CLOSING
        if (store_closing_time <= get_current_time())
        {
            pthread_exit(NULL);
        }

        sem_wait(&machines_semaphore);
        // LOOP THROUGH ALL THE MACHINES TO FIND A SUITABLE ONE
        for (int i = 0; i < N; i++)
        {
            // CHECK IF MACHINE CAN PREPARE THE ICE CREAM
            // if (check_suitable_machine(i, preparation_time))
            int end_time = list_of_machines[i]->end_time;
            int machine_running_time_left = end_time - get_current_time();

            if (machines_working[i] == 1 && machine_running_time_left >= preparation_time)
            {
                sem_wait(&list_of_semaphores_for_machines[i]);

                machine_id = i + 1;
                machines_working[i] = 2;

                printf(CYAN "Machine %d starts preparing ice cream %d of customer %d at %d seconds\n" RESET, machine_id, order_id, customer_id, get_current_time());

                machine_allocated = 1;
                break;
            }
        }

        if (machine_allocated)
        {
            break;
        }
        sem_post(&machines_semaphore);
        // TO SIMULATE WAITING TIME BEFORE RETRYING
        sleep(1);
    }

    // SIMULATE ICE CREAM PREPARATION IN THE MACHINE
    sleep(preparation_time);

    printf(BLUE "Machine %d completes preparing ice cream %d of customer %d at %d seconds(s)\n" RESET, machine_id, order_id, customer_id, get_current_time());

    // MACHINE BECOMES FREE AFTER 1 SECOND
    // sleep(1);
    // MARK ORDER AS COMPLETED
    order->order_completed = 1;

    // MAKE MACHINE IS AVAILABLE AGAIN
    machines_working[machine_id] = 1;
    sem_post(&list_of_semaphores_for_machines[machine_id]);
    sem_post(&machines_semaphore);

    // RETURN FROM MACHINE THREAD
    pthread_exit(NULL);
}

void *customer_thread_function(void *args)
{
    customer_t *customer = (customer_t *)args;
    int customer_id = customer->customer_id;
    // printf("l406 custid = %d\n", customer_id);
    int number_of_icecreams = customer->number_of_icecreams;

    // CREATE ORDER THREADS
    pthread_t *order_threads = (pthread_t *)calloc(number_of_icecreams, sizeof(pthread_t));
    // CREATE LIST OF ORDERS ARRAY FOR THREAD JOINING LATER
    order_t **list_of_orders = (order_t **)calloc(number_of_icecreams, sizeof(order_t *));

    for (int i = 0; i < number_of_icecreams; i++)
    {
        node_ptr icecream = customer->icecreams[i];
        list_of_orders[i] = create_order(i + 1, customer_id, icecream);
        pthread_create(&order_threads[i], NULL, machine_thread_function, (void *)list_of_orders[i]);
    }
    // FLAG TO CHECK IF ALL THE ORDERS ARE BEING COMPLETED
    int all_orders_completed_flag = 1;
    for (int i = 0; i < number_of_icecreams; i++)
    {
        pthread_join(order_threads[i], NULL);
        all_orders_completed_flag *= list_of_orders[i]->order_completed;
    }
    // HELPS ENSURE ATOMICITY - PREPARE ALL THE ICECREAMS, OR NONE OF THEM
    if (all_orders_completed_flag)
    {
        // ALL THE ORDERS HAVE BEEN COMPLETED
        printf(GREEN "Customer %d has collected their order(s) and left at %d second(s)\n" RESET, customer_id, get_current_time());
    }
    else
    {
        printf(RED "Customer %d was not serviced due to unavailability of machines\n" RESET, customer_id);
    }

    // FREEING ALLOCATED MEMORY
    for (int i = 0; i < number_of_icecreams; i++)
    {
        free(list_of_orders[i]);
    }
    free(order_threads);
    free(list_of_orders);

    // RETURN FROM THE THREAD
    pthread_exit(NULL);
}

int main()
{
    scanf("%d%d%d%d", &N, &K, &F, &T);

    list_of_machines = (machine_t **)calloc(N, sizeof(machine_t *));
    list_of_flavours = (flavour_t **)calloc(F, sizeof(flavour_t *));
    list_of_toppings = (topping_t **)calloc(T, sizeof(topping_t *));
    machines_working = (int *)calloc(N, sizeof(int));
    store_closing_time = 0;

    for (int i = 0; i < N; i++)
    {
        list_of_machines[i] = (machine_t *)calloc(1, sizeof(machine_t));
        int start_time, end_time;
        scanf("%d%d", &start_time, &end_time);
        list_of_machines[i]->machine_id = i + 1;
        list_of_machines[i]->start_time = start_time;
        list_of_machines[i]->end_time = end_time;

        // THE LAST MACHINE END TIME IS THE STORE CLOSING TIME
        if (end_time > store_closing_time)
        {
            store_closing_time = end_time;
        }
    }
    // STORE CLOSING TIME HAS BEEN UPDATED
    for (int i = 0; i < F; i++)
    {
        list_of_flavours[i] = (flavour_t *)calloc(1, sizeof(flavour_t));
        char *flavour_name = (char *)calloc(MAX_FLAVOUR_LENGTH, sizeof(char));
        int preparation_time;
        scanf("%s", flavour_name);
        scanf("%d", &preparation_time);
        list_of_flavours[i]->flavour_id = i + 1;
        list_of_flavours[i]->flavour_name = flavour_name;
        list_of_flavours[i]->preparation_time = preparation_time;
    }

    for (int i = 0; i < T; i++)
    {
        list_of_toppings[i] = (topping_t *)calloc(1, sizeof(topping_t));
        char *topping_name = (char *)calloc(MAX_TOPPING_LENGTH, sizeof(char));
        int quantity;
        scanf("%s", topping_name);
        scanf("%d", &quantity);
        list_of_toppings[i]->topping_id = i + 1;
        list_of_toppings[i]->topping_name = topping_name;
        list_of_toppings[i]->quantity = quantity;
    }

    // SET UP FOR THE SHARED VARIABLES
    init_time = time(NULL);
    customers_in_line = 0;
    sem_init(&machines_semaphore, 0, 0);
    list_of_semaphores_for_machines = (sem_t *)calloc(N, sizeof(sem_t));
    for (int i = 0; i < N; ++i)
    {
        sem_init(&list_of_semaphores_for_machines[i], 0, 1);
    }

    // RUN THE STORE THREAD
    pthread_create(&store_thread, NULL, store_thread_function, NULL);

    // RUN THE CUSTOMER THREADS
    while (1)
    {
        // TAKE CUSTOMER INPUT
        int customer_id;
        int arrival_time;
        int number_of_icecreams;
        scanf("%d%d%d", &customer_id, &arrival_time, &number_of_icecreams);

        // WAIT TILL THE CUSTOMER ARRIVES
        int wait_time = arrival_time - get_current_time();
        if (wait_time > 0)
        {
            sleep(wait_time);
        }

        // IF NUMBER OF CUSTOMERS EXCEEDS MAXIMUM SPACE IN THE STORE
        pthread_mutex_lock(&lock_customers_in_line);
        if (customers_in_line > K)
        {
            pthread_mutex_unlock(&lock_customers_in_line); // NEEDED LOCK FOR COMPARISON IN if CONDITION
            printf("Customer %d left due to unavailability of space at %d second(s)\n", customer_id, arrival_time);
            continue;
        }

        customers_in_line++;
        pthread_mutex_unlock(&lock_customers_in_line); // NEEDED LOCK FOR INCREMENTING NUMBER OF CUSTOMERS
        printf(WHITE "Customer %d enters at %d second(s)\n" RESET, customer_id, arrival_time);
        printf(YELLOW "Customer %d orders %d ice creams\n" RESET, customer_id, number_of_icecreams);

        node_ptr *icecreams = (node_ptr *)calloc(number_of_icecreams, sizeof(node_ptr));

        // ICE CREAM ORDERS OF THE CUSTOMER
        for (int i = 0; i < number_of_icecreams; i++)
        {
            // HANDLE NEW LINE CHARACTER
            char garbage;
            scanf("%c", &garbage);
            // TAKE INPUT
            char *input = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));

            scanf("%99[^\n]", input);

            // TAKE FLAVOUR STRING INPUT
            char *token;
            token = strtok_r(input, " \t\n", &input);

            // GET FLAVOUR ID
            int flavour_id = get_flavour_id(token);
            if (flavour_id == -1)
            {
                fprintf(stderr, "Invalid Flavour Input: '%s' is not an available flavour!", token);
                exit(EXIT_FAILURE);
            }

            // ADD FLAVOUR ID TO THE LIST
            icecreams[i] = create_node(flavour_id);

            // PRINT ICE CREAM NUMBER
            printf(YELLOW "Ice cream %d: " RESET, i + 1);
            printf(YELLOW "%s " RESET, token);
            token = strtok_r(input, " \t\n", &input);

            // GET TOPPINGS
            while (token != NULL)
            {
                // PRINT TOPPING
                printf(YELLOW "%s " RESET, token);
                // GET TOPPING ID
                int topping_id = get_topping_id(token);
                if (topping_id == -1)
                {
                    fprintf(stderr, "Invalid Topping Input: '%s' is not an available topping!", token);
                    exit(EXIT_FAILURE);
                }

                // INSERTING TOPPING TO THE LIST
                insert_node(icecreams[i], topping_id);

                // ACCESSING THE NEXT TOPPING
                token = strtok_r(NULL, " \t\n", &input);
            }
            printf("\n");
        }

        // CHECK IF WE ARE OUT OF TOPPINGS OR NOT
        int order_possible = can_fulfill_order(number_of_icecreams, icecreams);

        if (order_possible)
        {
            // CREATE CUSTOMER STRUCT
            customer_t customer;
            customer.customer_id = customer_id;
            customer.arrival_time = arrival_time;
            customer.number_of_icecreams = number_of_icecreams;
            customer.icecreams = icecreams;

            // CREATE CUSTOMER THREAD
            pthread_t customer_thread;
            pthread_create(&customer_thread, NULL, customer_thread_function, (void *)&customer);
        }
        else
        {
            printf(RED "Customer %d left at %d second(s) with an unfulfilled order\n" RESET, customer_id, get_current_time());
            customer_leaves_with_unfulfilled_order_handler();
        }
    }

    // // PRINTING ALL THE INPUTS
    // printf("Machines:\n");
    // printMachines();
    // printf("Flavours:\n");
    // printFlavours();
    // printf("Toppings:\n");
    // printToppings();

    // WE CAN NOT JOIN THE CUSTOMER THREADS AS WE DO NOT KNOW HOW MANY THREADS ARE BEING CREATED IN THE FIRST PLACE

    free(list_of_semaphores_for_machines);

    return 0;
}
