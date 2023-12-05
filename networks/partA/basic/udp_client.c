#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define SZ 1000000

int main()
{
    // CREATING SOCKET
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (client_socket < 0)
    {
        perror("Error in socket");
        exit(1);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    int size_of_server_address = sizeof(server_address);
    // SENDING MESSAGES FROM CLIENT
    char *buffer = (char *)calloc(SZ, sizeof(char));
    char *reply_buffer = (char *)calloc(SZ, sizeof(char));
    while (1)
    {
        printf("Send a message or quit using 'quit': ");
        fgets(buffer, SZ, stdin);

        sendto(client_socket, buffer, strlen(buffer), 0, (struct sockaddr *)&server_address, sizeof(server_address));
        if (strcmp(buffer, "quit\n") == 0)
        {
            printf("Client Exiting...\n");
            break;
        }

        recvfrom(client_socket, reply_buffer, SZ, 0, (struct sockaddr *)&server_address, &size_of_server_address);
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
