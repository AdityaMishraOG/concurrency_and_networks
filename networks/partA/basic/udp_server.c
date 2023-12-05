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
    int server_socket = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_socket < 0)
    {
        perror("Could not create socket");
        exit(1);
    }

    struct sockaddr_in server_address;
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = INADDR_ANY;

    // BINDING THE SOCKET
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Could not bind the socket");
        exit(1);
    }

    printf("UDP Server is listening on port %d...\n", PORT);

    struct sockaddr_in client_address;
    int size_of_client_address = sizeof(client_address);

    char *buffer = (char *)calloc(SZ, sizeof(char));
    while (1)
    {

        memset(buffer, 0, SZ);
        int bytes_received = recvfrom(server_socket, buffer, SZ, 0, (struct sockaddr *)&client_address, &size_of_client_address);
        buffer[bytes_received] = '\0';

        printf("Client: %s", buffer); // Print the entire received message

        if (strcmp(buffer, "quit\n") == 0)
        {
            printf("Client Exiting...\n");
            break;
        }

        // Send the entire received message back to the client
        sendto(server_socket, buffer, bytes_received, 0, (struct sockaddr *)&client_address, size_of_client_address);
    }
    free(buffer);
    printf("Connection ended\n");
    close(server_socket);
    return 0;
}
