#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8081
#define SZ 1024

int main()
{
    // CREATING THE SOCKET
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Could not create socket!");
        exit(1);
    }
    struct sockaddr_in server_socket_address;
    server_socket_address.sin_port = PORT;
    server_socket_address.sin_family = AF_INET;
    server_socket_address.sin_addr.s_addr = INADDR_ANY;

    // BINDING THE SOCKET
    if (bind(server_socket, (struct sockaddr *)&server_socket_address, sizeof(server_socket_address)) < 0)
    {
        perror("Could not bind socket");
        exit(1);
    }

    if (listen(server_socket, 15) == 0)
    {
        printf("Listening...\n");
    }
    else
    {
        perror("Error in listening");
        exit(1);
    }

    // ACCEPTING THE DATA PACKET FROM THE CLIENT
    struct sockaddr_in new_address;
    int size_of_new_address = sizeof(new_address);
    int new_socket = accept(server_socket, (struct sockaddr *)&new_address, &size_of_new_address);

    if (new_socket < 0)
    {
        perror("Could not accept data");
        exit(1);
    }

    char *buffer = (char *)calloc(SZ, sizeof(char));
    while (1)
    {
        memset(buffer, 0, SZ);
        recv(new_socket, buffer, SZ, 0);
        if (strcmp(buffer, "quit\n") == 0)
        {
            printf("Client Exiting...\n");
            break;
        }

        printf("Client: %s", buffer);
        send(new_socket, buffer, strlen(buffer), 0);
    }
    free(buffer);

    close(server_socket);
    close(new_socket);
    printf("Connection ended\n");
    return 0;
}
