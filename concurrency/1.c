#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#define MAX_COFFEE_NAME_LENGTH 20

#define WHITE "\x1b[37m"
#define YELLOW "\x1b[33m"
#define CYAN "\x1b[36m"
#define BLUE "\x1b[34m"
#define GREEN "\x1b[32m"
#define RED "\x1b[31m"
#define RESET "\x1b[0m"

typedef struct coffee_t coffee_t;
struct coffee_t
{
    int time;
    char *name;
    int coffee_id;
};

typedef struct customer_t customer_t;
struct customer_t
{
    int customer_id;
    int coffee_id;
    int coffee_preparation_time;
    int arrival_time;
    int tolerance_time;
};

int B_baristas;
int K_coffee_types;
int N_customers;
sem_t baristas;
time_t init_time;
int number_of_coffees_wasted = 0;
pthread_mutex_t mutex_coffees_wasted = PTHREAD_MUTEX_INITIALIZER;

coffee_t **list_of_coffees;
customer_t **list_of_customers;

time_t get_current_time()
{
    return time(NULL) - init_time;
}

char *get_coffee_name(int coffee_id)
{
    for (int i = 0; i < K_coffee_types; i++)
    {
        if (list_of_coffees[i]->coffee_id == coffee_id)
        {
            return list_of_coffees[i]->name;
        }
    }
    return NULL;
}

int article_for_coffee_name(char *coffee_name)
{
    char ch = coffee_name[0];
    if (ch == 'a' || ch == 'e' || ch == 'i' || ch == 'o' || ch == 'u')
        return 1;
    return 0;
}

void *customer_thread_function(void *args)
{
    customer_t *customer = (customer_t *)args;

    // SLEEP THREAD TILL ARRIVAL TIME
    time_t current_time = time(NULL) - init_time;
    if (current_time < customer->arrival_time)
        sleep(customer->arrival_time - current_time);

    // CUSTOMER ARRIVAL TIME
    time_t customer_arrival_time = time(NULL) - init_time;

    // GET NAME OF THE COFFEE THE CUSTOMER ORDERED
    char *coffee_name = (char *)calloc(MAX_COFFEE_NAME_LENGTH, sizeof(char));
    coffee_name = get_coffee_name(customer->coffee_id);

    // THROW ERROR IF THE COFFEE DOES NOT EXIST
    if (coffee_name == NULL)
    {
        fprintf(stderr, "Invalid Coffee Id: %d is not a valid Coffee Id!", customer->coffee_id);
        exit(EXIT_FAILURE);
    }
    // GRAMMAR
    int article = article_for_coffee_name(coffee_name);

    printf(WHITE "Customer %d arrives at %ld second(s)\n" RESET, customer->customer_id, customer_arrival_time);
    if (article)
        printf(YELLOW "Customer %d orders an %s\n" RESET, customer->customer_id, coffee_name);
    else
        printf(YELLOW "Customer %d orders a %s\n" RESET, customer->customer_id, coffee_name);

    // TIMEOUT AT TOLERANCE TIME
    struct timespec timeout;
    clock_gettime(CLOCK_REALTIME, &timeout);
    time_t reqd_time = customer->tolerance_time - (get_current_time() - customer->arrival_time);
    timeout.tv_sec += reqd_time + 1;
    // printf("reqdtime = %ld\n", reqd_time);

    // ATTEMPT TO ACQUIRE A BARISTA
    // IF CUSTOMER RUNS OUT OF PATIENCE AND HAS NOT ACQUIRED A BARISTA YET
    if (sem_timedwait(&baristas, &timeout) == -1)
    {
        // CUSTOMER LEAVES ON THE NEXT SECOND
        // printf("l108\n");
        // printf("curtime = %ld", time(NULL) - init_time);
        // sleep(1);
        printf(RED "Customer %d leaves without their order at %ld second(s)\n" RESET, customer->customer_id, time(NULL) - init_time);
        pthread_exit(NULL);
    }

    // GET BARISTA NUMBER FROM THE VALUE OF THE SEMAPHORE
    int semaphore_value;
    sem_getvalue(&baristas, &semaphore_value);
    int barista_number = B_baristas - semaphore_value;

    // BARISTA PREPARES THE ORDER AT LEAST AFTER A DELAY OF 1 SECOND
    int barista_preparation_start_time = time(NULL) - init_time;
    if (customer_arrival_time == barista_preparation_start_time)
    {
        sleep(1);
        barista_preparation_start_time++;
    }

    // GET COFFEE PREPARATION TIME
    int coffee_id = customer->coffee_id;
    int coffee_preparation_time = customer->coffee_preparation_time;
    // printf("l129\n");
    // BARISTA STARTS PREPARING THE ORDER
    printf(CYAN "Barista %d begins preparing the order of customer %d at %ld second(s)\n" RESET, barista_number, customer->customer_id, time(NULL) - init_time);

    // IF CUSTOMER RUNS OUT OF PATIENCE AFTER ACQUIRING THE BARISTA
    if (barista_preparation_start_time + coffee_preparation_time >= customer_arrival_time + customer->tolerance_time)
    {
        // COFFEE IS WASTED
        pthread_mutex_lock(&mutex_coffees_wasted);
        number_of_coffees_wasted++;
        pthread_mutex_unlock(&mutex_coffees_wasted);

        // CUSTOMER WAITS FOR SOME TIME BEFORE LOSING PATIENCE
        time_t customer_wait_time = customer_arrival_time + customer->tolerance_time - barista_preparation_start_time;
        sleep(customer_wait_time);

        // CUSTOMER LEAVES THE NEXT SECOND
        sleep(1);
        printf(RED "Customer %d leaves without their order at %ld second(s)\n" RESET, customer->customer_id, time(NULL) - init_time);

        // BARISTA PREPARES THE COFFEE NEVERTHELESS
        time_t barista_waste_time = barista_preparation_start_time + coffee_preparation_time - customer_arrival_time - customer->tolerance_time;

        sleep(barista_waste_time - 1);

        printf(BLUE "Barista %d completes the order of customer %d at %ld second(s)\n" RESET, barista_number, customer->customer_id, time(NULL) - init_time);

        //  FREE THE BARISTA AFTER A SECOND'S DELAY
        sleep(1);
        sem_post(&baristas);

        // TERMINATE THE TRHREAD
        pthread_exit(NULL);
    }

    sleep(coffee_preparation_time);

    // COFFEE IS NOW PREPARED
    printf(BLUE "Barista %d completes the order of customer %d at %ld second(s)\n" RESET, barista_number, customer->customer_id, time(NULL) - init_time);
    // CUSTOMER LEAVES
    printf(GREEN "Customer %d leaves with their order at %ld second(s)\n" RESET, customer->customer_id, time(NULL) - init_time);

    // FREE THE BARISTA AFTER A SECOND'S DELAY
    sleep(1);
    sem_post(&baristas);
    pthread_exit(NULL);
}

int main()
{
    scanf("%d%d%d", &B_baristas, &K_coffee_types, &N_customers);

    // TAKING COFFEE INPUTS
    list_of_coffees = (coffee_t **)calloc(K_coffee_types, sizeof(coffee_t *));
    for (int i = 0; i < K_coffee_types; i++)
    {
        char *name = (char *)calloc(MAX_COFFEE_NAME_LENGTH, sizeof(char));
        scanf("%s", name);
        int time;
        scanf("%d", &time);

        list_of_coffees[i] = (coffee_t *)calloc(1, sizeof(coffee_t));
        list_of_coffees[i]->name = name;
        list_of_coffees[i]->time = time;
        list_of_coffees[i]->coffee_id = i + 1;
    }

    // TAKING CUSTOMER INPUTS
    list_of_customers = (customer_t **)calloc(N_customers, sizeof(customer_t *));
    for (int i = 0; i < N_customers; i++)
    {
        int customer_id;
        int coffee_id;
        int coffee_preparation_time;
        int arrival_time;
        int tolerance_time;
        char *coffee_name = (char *)calloc(MAX_COFFEE_NAME_LENGTH, sizeof(char));

        scanf("%d%s%d%d", &customer_id, coffee_name, &arrival_time, &tolerance_time);
        int j;
        // FINDING THE COFFEE ID FOR THE ENTERED COFFEE NAME
        for (j = 0; j < K_coffee_types; j++)
        {
            coffee_t *current_coffee = list_of_coffees[j];
            if (strcmp(current_coffee->name, coffee_name) == 0)
            {
                break;
            }
            // printf("name1 = %s, name2 = %s\n", current_coffee->name, coffee_name);
            // printf("len1 = %ld, len2 = %ld\n", strlen(current_coffee->name), strlen(coffee_name));
        }
        // COFFEE NAME NOT FOUND
        if (j == K_coffee_types)
        {
            perror("Coffee name not found in list of coffee names");
            exit(1);
        }

        coffee_id = list_of_coffees[j]->coffee_id;
        coffee_preparation_time = list_of_coffees[j]->time;

        list_of_customers[i] = (customer_t *)calloc(1, sizeof(customer_t));
        list_of_customers[i]->customer_id = customer_id;
        list_of_customers[i]->coffee_id = coffee_id;
        list_of_customers[i]->coffee_preparation_time = coffee_preparation_time;
        list_of_customers[i]->arrival_time = arrival_time;
        list_of_customers[i]->tolerance_time = tolerance_time;
    }

    // CREATING BARISTA SEMAPHORE
    sem_init(&baristas, 0, B_baristas);

    // CREATING CUSTOMERS
    pthread_t *customer_threads = (pthread_t *)calloc(N_customers, sizeof(pthread_t));
    init_time = time(NULL);
    for (int i = 0; i < N_customers; i++)
    {
        pthread_create(&customer_threads[i], NULL, customer_thread_function, (void *)list_of_customers[i]);
    }

    // JOINING CUSTOMER THREADS
    for (int i = 0; i < N_customers; i++)
    {
        pthread_join(customer_threads[i], NULL);
    }
    if (number_of_coffees_wasted == 1)
        printf("\n%d coffee wasted\n", number_of_coffees_wasted);
    else
        printf("\n%d coffees wasted\n", number_of_coffees_wasted);

    sem_destroy(&baristas);
    return 0;
}
