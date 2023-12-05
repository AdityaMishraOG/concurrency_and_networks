#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

int main()
{
    char *ip = "127.0.0.1";
    int port1 = 8082; // Port for clientA
    int port2 = 8083; // Port for clientB

    int server_sock1, server_sock2, client_sock1, client_sock2;
    struct sockaddr_in server_addr1, server_addr2, client_addr1, client_addr2;
    socklen_t addr_size1, addr_size2;
    char buffer1[1024], buffer2[1024];
    int n;

    server_sock1 = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock1 < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }

    server_sock2 = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock2 < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }

    printf("[+]TCP server sockets created.\n");

    memset(&server_addr1, '\0', sizeof(server_addr1));
    server_addr1.sin_family = AF_INET;
    server_addr1.sin_port = htons(port1);
    server_addr1.sin_addr.s_addr = inet_addr(ip);

    memset(&server_addr2, '\0', sizeof(server_addr2));
    server_addr2.sin_family = AF_INET;
    server_addr2.sin_port = htons(port2);
    server_addr2.sin_addr.s_addr = INADDR_ANY;
    //  inet_addr(ip);

    n = bind(server_sock1, (struct sockaddr *)&server_addr1, sizeof(server_addr1));
    if (n < 0)
    {
        perror("[-]Bind error");
        exit(1);
    }

    n = bind(server_sock2, (struct sockaddr *)&server_addr2, sizeof(server_addr2));
    if (n < 0)
    {
        perror("[-]Bind error");
        exit(1);
    }

    printf("[+]Bind to port numbers: %d and %d\n", port1, port2);

    listen(server_sock1, 15);
    listen(server_sock2, 15);

    printf("Listening for clients...\n");

    addr_size1 = sizeof(client_addr1);
    client_sock1 = accept(server_sock1, (struct sockaddr *)&client_addr1, &addr_size1);
    printf("[+]Client A connected.\n");

    addr_size2 = sizeof(client_addr2);
    client_sock2 = accept(server_sock2, (struct sockaddr *)&client_addr2, &addr_size2);
    printf("[+]Client B connected.\n");

    while (1)
    {

        char choiceA[10], choiceB[10];
        char result[10];
        for (int i = 0; i < 10; i++)
        {
            choiceA[i] = '\0';
            choiceB[i] = '\0';
            result[i] = '\0';
        }

        // Receive choices from clientA
        recv(client_sock1, choiceA, sizeof(choiceA), 0);

        // Receive choices from clientB
        recv(client_sock2, choiceB, sizeof(choiceB), 0);
        choiceA[strlen(choiceA)] = '\0';
        choiceB[strlen(choiceB)] = '\0';

        // Determine the winner
        char game_result;
        int invalid_input_flag = 0;

        if (strcmp(choiceA, "rock\n") != 0 && strcmp(choiceA, "paper\n") != 0 && strcmp(choiceA, "scissors\n") != 0)
        {
            invalid_input_flag += 1;
        }
        if (strcmp(choiceB, "rock\n") != 0 && strcmp(choiceB, "paper\n") != 0 && strcmp(choiceB, "scissors\n") != 0)
        {
            invalid_input_flag += 2;
        }

        if (strcmp(choiceA, choiceB) == 0 && invalid_input_flag == 0)
        {
            game_result = 'T'; // Tie
        }
        else if ((strcmp(choiceA, "rock\n") == 0 && strcmp(choiceB, "scissors\n") == 0) || (strcmp(choiceA, "paper\n") == 0 && strcmp(choiceB, "rock\n") == 0) || (strcmp(choiceA, "scissors\n") == 0 && strcmp(choiceB, "paper\n") == 0))
        {
            game_result = '1'; // Player A wins
        }
        else if ((strcmp(choiceB, "rock\n") == 0 && strcmp(choiceA, "scissors\n") == 0) || (strcmp(choiceB, "paper\n") == 0 && strcmp(choiceA, "rock\n") == 0) || (strcmp(choiceB, "scissors\n") == 0 && strcmp(choiceA, "paper\n") == 0))
        {
            game_result = '2'; // Player B wins
        }
        else
        {
            if (invalid_input_flag == 1)
                game_result = 'e';
            else if (invalid_input_flag == 2)
                game_result = 'f';
            else
                game_result = 'g';
        }
        // printf("game_result = %c\n", game_result);

        // Send the result back to both clients
        send(client_sock1, &game_result, sizeof(game_result), 0);
        send(client_sock2, &game_result, sizeof(game_result), 0);

        // Prompt for another game
        int close_game_flag = 0;
        char play_again1[10];
        char play_again2[10];

        recv(client_sock1, play_again1, sizeof(play_again1), 0);
        recv(client_sock2, play_again2, sizeof(play_again2), 0);

        if (strcmp(play_again1, "no\n") == 0)
        {
            printf("Game over. Exiting...\n");
            close_game_flag = 1;
        }
        if (strcmp(play_again2, "no\n") == 0)
        {
            printf("Game over. Exiting...\n");
            close_game_flag = 1;
        }
        // SEND CLOSING REQUEST TO THE CLIENTS
        send(client_sock1, &close_game_flag, sizeof(close_game_flag), 0);
        send(client_sock2, &close_game_flag, sizeof(close_game_flag), 0);
        // CLOSING CONNECTION
        if (close_game_flag)
        {
            break;
        }
    }
    close(client_sock1);
    close(client_sock2);

    close(server_sock1);
    close(server_sock2);

    printf("[+]Clients disconnected.\n\n");

    return 0;
}

// CHATGPT REFERENCE
// https://chat.openai.com/share/1eb2c8b4-b7bb-40d3-aa2d-e02a59c2efa0