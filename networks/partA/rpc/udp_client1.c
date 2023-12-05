#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8082
#define SZ 1024

int main()
{
    int client_socket = socket(AF_INET, SOCK_DGRAM, 0); // Use SOCK_DGRAM for UDP
    if (client_socket < 0)
    {
        perror("Could not create socket!");
        exit(1);
    }

    struct sockaddr_in server_socket_address;
    server_socket_address.sin_family = AF_INET;
    server_socket_address.sin_port = htons(SERVER_PORT);
    server_socket_address.sin_addr.s_addr = inet_addr(SERVER_IP);

    char buffer[SZ];

    while (1)
    {
        char choice[10];

        // Prompt for user input (Rock, Paper, Scissors)
        printf("Enter your choice (rock, paper, or scissors): ");
        fgets(choice, sizeof(choice), stdin);

        // Send the choice to the server
        sendto(client_socket, choice, sizeof(choice), 0, (struct sockaddr *)&server_socket_address, sizeof(server_socket_address));

        // Receive and display the result from the server
        char result;
        socklen_t addr_len = sizeof(server_socket_address);
        recvfrom(client_socket, &result, sizeof(result), 0, (struct sockaddr *)&server_socket_address, &addr_len);

        // printf("result = %c\n", result);
        if (result == 'T')
        {
            printf("Draw\n");
        }
        else if (result == '1')
        {
            printf("Win\n");
        }
        else if (result == '2')
        {
            printf("Lost\n");
        }
        else if (result == 'e' || result == 'g')
        {
            printf("You entered an invalid input\n");
        }
        else if (result == 'f')
        {
            printf("The Other Client entered an invalid input\n");
        }
        // Prompt for another game
        int close_game_flag = 0;
        char play_again[10];
        printf("Do you want to play another game? (yes/no): ");
        fgets(play_again, sizeof(play_again), stdin);

        // Send the play again choice to the server
        sendto(client_socket, play_again, sizeof(play_again), 0, (struct sockaddr *)&server_socket_address, sizeof(server_socket_address));

        // RECEIVE CLOSE GAME FLAG
        recvfrom(client_socket, &close_game_flag, sizeof(close_game_flag), 0, (struct sockaddr *)&server_socket_address, &addr_len);
        if (close_game_flag)
        {
            printf("Quitting game...\n");
            break;
        }
    }

    close(client_socket);

    return 0;
}

// CHATGPT REFERENCE
// https://chat.openai.com/share/1eb2c8b4-b7bb-40d3-aa2d-e02a59c2efa0
