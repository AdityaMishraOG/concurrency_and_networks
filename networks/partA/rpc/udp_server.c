#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define PORT1 8082
#define PORT2 8083
#define SZ 1024

int main()
{
    int server_sock1, server_sock2;
    struct sockaddr_in server_addr1, server_addr2, client_addr1, client_addr2;
    socklen_t addr_size1, addr_size2;
    char buffer1[1024], buffer2[1024];
    int n;

    server_sock1 = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock1 < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }

    server_sock2 = socket(AF_INET, SOCK_DGRAM, 0);
    if (server_sock2 < 0)
    {
        perror("[-]Socket error");
        exit(1);
    }

    memset(&server_addr1, '\0', sizeof(server_addr1));
    server_addr1.sin_family = AF_INET;
    server_addr1.sin_port = htons(PORT1); // Port for clientA
    server_addr1.sin_addr.s_addr = inet_addr(SERVER_IP);

    memset(&server_addr2, '\0', sizeof(server_addr2));
    server_addr2.sin_family = AF_INET;
    server_addr2.sin_port = htons(PORT2); // Port for clientB
    server_addr2.sin_addr.s_addr = inet_addr(SERVER_IP);

    n = bind(server_sock1, (struct sockaddr *)&server_addr1, sizeof(server_addr1));
    if (n < 0)
    {
        perror("[-]Bind error for clientA");
        exit(1);
    }

    n = bind(server_sock2, (struct sockaddr *)&server_addr2, sizeof(server_addr2));
    if (n < 0)
    {
        perror("[-]Bind error for clientB");
        exit(1);
    }

    printf("[+]UDP server sockets created and bound to ports %d and %d for clientA and clientB.\n", PORT1, PORT2);

    addr_size1 = sizeof(client_addr1);
    addr_size2 = sizeof(client_addr2);

    // Receive data from clientA
    // recvfrom(server_sock1, buffer1, sizeof(buffer1), 0, (struct sockaddr *)&client_addr1, &addr_size1);
    printf("[+]Client A connected.\n");

    // Receive data from clientB
    // recvfrom(server_sock2, buffer2, sizeof(buffer2), 0, (struct sockaddr *)&client_addr2, &addr_size2);
    printf("[+]Client B connected.\n");

    // Rest of your code for processing and responding to clients goes here...

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

        // printf("l84 receiving from clients...\n");

        // Receive choices from clientA
        recvfrom(server_sock1, choiceA, sizeof(choiceA), 0, (struct sockaddr *)&client_addr1, &addr_size1);

        // Receive choices from clientB
        recvfrom(server_sock2, choiceB, sizeof(choiceB), 0, (struct sockaddr *)&client_addr2, &addr_size2);
        choiceA[strlen(choiceA)] = '\0';
        choiceB[strlen(choiceB)] = '\0';

        // printf("inputs received l92\n");
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

        if (strcmp(choiceA, choiceB) == 0 && !invalid_input_flag)
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
        printf("game_result = %c\n", game_result);

        // Send the result back to both clients
        sendto(server_sock1, &game_result, sizeof(game_result), 0, (struct sockaddr *)&client_addr1, addr_size1);
        sendto(server_sock2, &game_result, sizeof(game_result), 0, (struct sockaddr *)&client_addr2, addr_size2);

        // Prompt for another game
        int close_game_flag = 0;
        char play_again1[10];
        char play_again2[10];

        recvfrom(server_sock1, play_again1, sizeof(play_again1), 0, (struct sockaddr *)&client_addr1, &addr_size1);
        recvfrom(server_sock2, play_again2, sizeof(play_again2), 0, (struct sockaddr *)&client_addr2, &addr_size2);
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
        sendto(server_sock1, &close_game_flag, sizeof(close_game_flag), 0, (struct sockaddr *)&client_addr1, addr_size1);
        sendto(server_sock2, &close_game_flag, sizeof(close_game_flag), 0, (struct sockaddr *)&client_addr2, addr_size2);
        // CLOSING CONNECTION
        if (close_game_flag)
        {
            break;
        }
    }
    close(server_sock1);
    close(server_sock2);

    return 0;
}

// CHATGPT REFERENCE
// https://chat.openai.com/share/1eb2c8b4-b7bb-40d3-aa2d-e02a59c2efa0