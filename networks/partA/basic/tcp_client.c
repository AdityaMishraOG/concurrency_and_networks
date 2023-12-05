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
    int client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (client_socket < 0)
    {
        perror("Could not create socket!");
        exit(1);
    }

    struct sockaddr_in server_socket_address;
    server_socket_address.sin_port = PORT;
    server_socket_address.sin_family = AF_INET;
    server_socket_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    // CONNECTING THE CLIENT TO THE SERVER
    if (connect(client_socket, (struct sockaddr *)&server_socket_address, sizeof(server_socket_address)) < 0)
    {
        perror("Could not connect");
        exit(1);
    }

    // SENDING THE DATA PACKET FROM THE CLIENT
    char *buffer = (char *)calloc(SZ, sizeof(char));
    char *reply_buffer = (char *)calloc(SZ, sizeof(char));
    while (1)
    {
        printf("Enter a message or quit using 'quit': ");
        fgets(buffer, SZ, stdin);

        send(client_socket, buffer, strlen(buffer), 0);
        if (strcmp(buffer, "quit\n") == 0)
        {
            printf("quitting...\n");
            break;
        }

        recv(client_socket, reply_buffer, SZ, 0);
        printf("Server: %s", reply_buffer);
        // RESETTING THE BUFFERS
        memset(buffer, 0, SZ);
        memset(reply_buffer, 0, SZ);
    }
    free(buffer);
    free(reply_buffer);
    close(client_socket);

    printf("Connection ended\n");
    return 0;
}
