#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <fcntl.h>

#define PORT 8080
#define SZ 1000000
#define SEGMENT_SIZE 3

struct Sache
{
    int id;
    struct timeval init_time;
    char *data;
};
typedef struct Sache Sache;

Sache *createSache(int id, char *str)
{
    Sache *ret = (Sache *)calloc(1, sizeof(Sache));
    ret->data = str;
    ret->id = id;
    gettimeofday(&(ret->init_time), NULL);
    return ret;
}

typedef struct Node *LinkedList;
typedef struct Node *PtrNode;
struct Node
{
    Sache *value;
    PtrNode next;
};

PtrNode createNode(int id, char *str)
{
    PtrNode ret = (PtrNode)malloc(sizeof(struct Node));
    ret->value = createSache(id, str);
    ret->next = NULL;
    return ret;
}

PtrNode createNode2(Sache *sache)
{
    PtrNode ret = (PtrNode)malloc(sizeof(struct Node));
    ret->value = sache;
    ret->next = NULL;
    return ret;
}

void Push(LinkedList Head, Sache *sache)
{
    int already_present_flag = 0;
    // first check if sache is already present in head or not
    PtrNode cur = Head->next;
    while (cur != NULL)
    {
        if (cur->value->id == sache->id)
        {
            already_present_flag = 1;
            return;
        }
        cur = cur->next;
    }

    PtrNode new = createNode2(sache);

    PtrNode first = Head->next;
    Head->next = new;
    new->next = first;
}

void Print(LinkedList Head)
{
    PtrNode cur = Head->next;
    if (cur == NULL)
    {
        printf("_\n");
        return;
    }
    while (cur != NULL)
    {
        printf("%d ", cur->value->id);
        cur = cur->next;
    }
    printf("\n");
}

bool break_condition(int number_of_segments, bool *received)
{
    for (int i = 0; i < number_of_segments; i++)
    {
        if (received[i] == false)
        {
            return false;
        }
    }
    return true;
}

int main()
{

    // Create a UDP socket
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Configure server address
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket to the server address
    if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Binding failed");
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    struct sockaddr_in client_addr;
    int client_addr_len = sizeof(client_addr);

    printf("UDP server is listening on port %d...\n", PORT);

    // Define a variable to store the received int value
    int number_of_segments;

    // Receiving the number of segments from the client
    ssize_t recv_bytes = recvfrom(server_socket, &number_of_segments, sizeof(number_of_segments), 0, (struct sockaddr *)&client_addr, &client_addr_len);
    if (recv_bytes < 0)
    {
        perror("Receive failed");
        exit(EXIT_FAILURE);
    }

    // Now, 'number_of_segments' contains the integer value sent by the client
    // printf("Number of segments to be received from client: %d\n", number_of_segments);

    char **array_of_segments = (char **)malloc(number_of_segments * sizeof(char *));
    bool *received = (bool *)calloc(number_of_segments, sizeof(bool));

    while (!break_condition(number_of_segments, received))
    {
        // Receive Sache from the client
        // Sache *current_sache = createEmptySache();
        // current_sache->data = (char *)calloc(SEGMENT_SIZE + 1, sizeof(char));
        char *received_data = (char *)calloc(SZ, sizeof(char));
        int recv_bytes = recvfrom(server_socket, received_data, SZ, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (recv_bytes < 0)
        {
            perror("Receive failed");
            continue;
        }

        // Extract the string and add it to the string array
        int id;
        time_t init_time;
        char *data_string;

        char *ptr = received_data;
        memcpy(&id, ptr, sizeof(int));
        ptr += sizeof(int);
        memcpy(&init_time, ptr, sizeof(time_t));
        ptr += sizeof(time_t);
        data_string = strdup(ptr);

        // printf("id = %d\n", id);
        // printf("data_string = %s\n", data_string);
        received[id] = true;

        array_of_segments[id] = strdup(data_string);

        // Send an acknowledgement back to the client
        int acknowledgement = id;

        ssize_t send_bytes = sendto(server_socket, &acknowledgement, sizeof(acknowledgement), 0, (struct sockaddr *)&client_addr, client_addr_len);
        if (send_bytes < 0)
        {
            perror("Could not send ACK");
            continue;
        }

        printf("ACK sent for segment %d\n", id);
    }

    char *received_string = (char *)calloc(SZ, sizeof(char));
    int k = 0;
    for (int i = 0; i < number_of_segments; i++)
    {
        for (int j = 0; j < SEGMENT_SIZE; j++)
        {
            received_string[k] = array_of_segments[i][j];
            k++;
        }
    }

    printf("Received string is: %s\n", received_string);

    //////////////////////////////////////////////////////////////
    //      PART 2       SEND A STRING TO THE CLIENT     ////////
    /////////////////////////////////////////////////////////////
    // Get the current file status flags
    int flags;
    if ((flags = fcntl(server_socket, F_GETFL, 0)) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }

    // Set the non-blocking flag using bitwise OR
    flags |= O_NONBLOCK;

    // Set the modified flags back to the file descriptor
    if (fcntl(server_socket, F_SETFL, flags) == -1)
    {
        perror("fcntl");
        exit(EXIT_FAILURE);
    }
    // Getting input string
    char *str = (char *)calloc(SZ, sizeof(char));
    printf("Enter string: ");
    fgets(str, SZ, stdin);
    str[strlen(str) - 1] = '\0';
    // printf("strlen(str) = %ld\n", strlen(str));

    // Chopping the strings into smaller pieces
    int fractional_flag = (sizeof(str) % (sizeof(char) * SEGMENT_SIZE));
    number_of_segments = strlen(str) / SEGMENT_SIZE + (fractional_flag != 0);
    // printf("number_of_segments = %d\n", number_of_segments);

    // Sending the number of segments the server should be prepared to receive
    int ret = sendto(server_socket, &number_of_segments, sizeof(number_of_segments), 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
    if (ret < 0)
    {
        perror("Could not send message");
        exit(EXIT_FAILURE);
    }
    // Making an array to check which packets have been sent
    bool *sent = (bool *)calloc(number_of_segments, sizeof(bool));
    // Initiating Queue which holds the segments
    LinkedList Head = createNode(-2, "DUMMY");

    // printf("entering for loop...\n");
    for (int i = 0; i < number_of_segments; i++)
    {
        // printf("i = %d\n", i);
        // printf("LinkedList is: ");
        // Print(Head);
        if (i)
        {
            PtrNode cur = Head->next;

            while (cur != NULL)
            {
                struct timeval tv;
                gettimeofday(&tv, NULL);

                if (tv.tv_usec - cur->value->init_time.tv_usec > 100)
                {
                    printf("No ACK received. Resend segment %d\n", cur->value->id);
                    i = cur->value->id;
                    break;
                }
                cur = cur->next;
            }
            // printf("i has been updated to %d\n", i);
        }
        if (sent[i])
        {
            continue;
        }
        char *data_string = (char *)calloc(SZ, sizeof(char));
        int i1 = i * SEGMENT_SIZE;
        int i2;
        if (i == number_of_segments - 1)
        {
            i2 = i1 + fractional_flag;
        }
        else
        {
            i2 = i1 + SEGMENT_SIZE;
        }
        int k = 0;
        int j = i1;
        while (j != i2)
        {
            data_string[k] = str[j];
            j++;
            k++;
        }
        data_string[k] = '\0';
        // printf("data_string = %s\n", data_string);

        Sache *cur_sache = createSache(i, data_string);
        // Calculate the size needed for serialization
        int data_size = strlen(cur_sache->data) + 1;
        int serialized_size = sizeof(int) + sizeof(time_t) + data_size;
        char *serialized_data = (char *)malloc(serialized_size);
        if (serialized_data == NULL)
        {
            perror("Memory allocation failed");
            exit(EXIT_FAILURE);
        }

        // Serialize the structure into the buffer
        char *ptr = serialized_data;
        memcpy(ptr, &cur_sache->id, sizeof(int));
        ptr += sizeof(int);
        memcpy(ptr, &cur_sache->init_time, sizeof(time_t));
        ptr += sizeof(time_t);
        strcpy(ptr, cur_sache->data);

        int ret = sendto(server_socket, serialized_data, serialized_size, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
        if (ret < 0)
        {
            perror("Could not send segment");
            exit(EXIT_FAILURE);
        }

        Push(Head, cur_sache);
        free(data_string);
        free(serialized_data);

        // Declare a structure to specify a timeout duration
        struct timeval timeout_for_select;
        timeout_for_select.tv_sec = 0;      // Set the seconds part of the timeout to 0 (no full seconds)
        timeout_for_select.tv_usec = 50000; // Set the microseconds part of the timeout to 50000 (50 milliseconds)

        // Declare a set to manage file descriptors for the 'select' function
        fd_set readfds;
        FD_ZERO(&readfds);               // Clear the 'readfds' set initially (no file descriptors in it)
        FD_SET(server_socket, &readfds); // Add 'sockfd' to the 'readfds' set (monitor 'sockfd' for readability)

        // Use the 'select' function to wait for data or a timeout
        int select_status = select(server_socket + 1, &readfds, NULL, NULL, &timeout_for_select);

        // Check the result of the 'select' call
        if (select_status > 0)
        {
            // 'sockfd' became readable, meaning data is ready to be read
            // Perform read operations on 'sockfd' here
            // int size_of_client_address = sizeof(client);
            client_addr_len = sizeof(client_addr);
            int segment_num;
            int ret = recvfrom(server_socket, &segment_num, sizeof(segment_num), 0, (struct sockaddr *)&client_addr, &client_addr_len);
            if (ret < 0)
            {
                perror("Could not receive message");
                exit(EXIT_FAILURE);
            }
            printf("ACK for segment %d\n", segment_num);
            // Remove from Queue if it is not empty
            if (Head->next != NULL)
            {
                PtrNode cur = Head->next;
                PtrNode prev = Head;
                // printf("yo\n");
                while (cur != NULL)
                {
                    if (cur->value->id == segment_num)
                    {
                        prev->next = cur->next;
                        free(cur);
                        cur = prev->next;
                    }
                    if (cur == NULL)
                    {
                        break;
                    }
                    cur = cur->next;
                    prev = prev->next;
                }
            }

            sent[segment_num] = 1;
        }
        else if (select_status == 0)
        {
            // Timeout occurred, no data became available within the specified timeout
            // You can handle this case, e.g., by taking appropriate action or retrying
            continue;
        }
        else
        {
            // An error occurred in 'select'
            // Handle the error, e.g., by printing an error message or closing the socket
            perror("The socket is closed, or is in an error state");
            exit(EXIT_FAILURE);
        }
    }
    if (Head->next != NULL)
    {
        PtrNode cur = Head->next;
        while (cur != NULL)
        {
            Sache *cur_sache = cur->value;
            int i = cur_sache->id;
            char *data_string = strdup(cur_sache->data);

            // Calculate the size needed for serialization
            int data_size = strlen(cur_sache->data) + 1;
            int serialized_size = sizeof(int) + sizeof(time_t) + data_size;
            char *serialized_data = (char *)malloc(serialized_size);
            if (serialized_data == NULL)
            {
                perror("Memory allocation failed");
                exit(EXIT_FAILURE);
            }

            // Serialize the structure into the buffer
            char *ptr = serialized_data;
            memcpy(ptr, &cur_sache->id, sizeof(int));
            ptr += sizeof(int);
            memcpy(ptr, &cur_sache->init_time, sizeof(time_t));
            ptr += sizeof(time_t);
            strcpy(ptr, cur_sache->data);

            int ret = sendto(server_socket, serialized_data, serialized_size, 0, (struct sockaddr *)&client_addr, sizeof(client_addr));
            if (ret < 0)
            {
                perror("Could not send segment");
                exit(EXIT_FAILURE);
            }

            cur = cur->next;
        }
    }
    close(server_socket);

    return 0;
}
