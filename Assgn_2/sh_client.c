/*
 * NAME: NIKHIL SARASWAT
 * ROLL NO.: 20CS10039
 * ASSIGNMENT NO.: 2
 * PROBLEM: 2
 * FILE NAME: sh_client.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#ifndef PORT // if port is not defined, define it to 5566
#define PORT 20000
#endif
#define IP "127.0.0.1" // IP address of the server
#define CHUNK_SIZE 45  // maximum size of the chunk that can be sent or received


void send_output(int sock, char *output)
{
    int len = strlen(output); // length of the string
    int x = 0;
    while (1)
    {
        int mini = CHUNK_SIZE;        // minimum of the chunk size and the remaining length
        if (len + 1 - x < CHUNK_SIZE) // if remaining length is less than chunk size
        {
            mini = len + 1 - x;
        }
        int n = send(sock, output + x, mini, 0); // send the chunk
        if (n < 0)
        {
            perror("[-]Error in send\n"); // if error in sending
            exit(1);
        }
        x += n;
        if (x >= len)
        {
            break; // if all the string is sent
        }
    }
}

void receive_input(int sock, char *input)
{
    memset(input, '\0', (size_t)sizeof(input));
    int i = 0;
    while (1)
    {
        char buf[CHUNK_SIZE];
        memset(buf, 0, CHUNK_SIZE);
        int bytesReceived = recv(sock, buf, CHUNK_SIZE, 0);
        if (bytesReceived < 0)
        {
            perror("[-]Error in recv\n");
            exit(1);
        }
        else if (bytesReceived == 0)
        {
            break;
        }
        int j = 0;
        while (j < bytesReceived)
        {
            input[i] = buf[j];
            i++;
            j++;
            if (i == 1023)
            {
                input[i] = '\0';
                printf("%s", input);
                memset(input, '\0', (size_t)sizeof(input));
                i = 0;
            }
        }
        if (buf[bytesReceived - 1] == '\0')
        {
            break;
        }
    }
    input[i] = '\0';
}

void remove_spaces(char *str)
{
    // remove spaces from the string at beginning and end
    int i = 0;
    while (str[i] == ' ')
    {
        i++;
    }
    int j = 0;
    while (str[i] != '\0')
    {
        str[j] = str[i];
        i++;
        j++;
    }
    str[j] = '\0';
    i = strlen(str) - 1;
    while (str[i] == ' ')
    {
        i--;
    }
    str[i + 1] = '\0';
}

int main()
{
    int sock;
    struct sockaddr_in addr;
    char buffer[1024];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror(" [-]Error in sock\n");
        exit(1);
    }
    printf("[+]Socket created successfully\n");

    // set the server_addr struct with all zeros
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);

    // connect the socket to the server_addr
    int status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0)
    {
        perror("[-]Error in connect\n");
        exit(1);
    }
    printf("[+]Connection successful to port=%d and IP=%s\n", PORT, IP);

    receive_input(sock, buffer);
    printf("%s", buffer);
    memset(buffer, '\0', sizeof(buffer));
    scanf("%s", buffer);
    // send to the server
    int bytesSent = send(sock, buffer, strlen(buffer) + 1, 0);
    if (bytesSent < 0)
    {
        perror("[-]Error in send\n");
        exit(1);
    }
    receive_input(sock, buffer);
    printf("%s\n", buffer);
    if (strcmp(buffer, "NOT-FOUND") == 0)
    {
        close(sock);
        return 0;
    }
    getchar();
    while (1)
    {
        printf("Enter a shell command: ");
        memset(buffer, '\0', sizeof(buffer));
        // take input from the user until the user enters enter
        scanf("%[^\n]%*c", buffer);
        remove_spaces(buffer);
        send_output(sock, buffer);
        if (strcmp(buffer, "exit") == 0)
        {
            break;
        }
        receive_input(sock, buffer);
        if (strcmp(buffer, "$$$$") == 0)
        {
            printf("Invalid command\n");
        }
        else if (strcmp(buffer, "####") == 0)
        {
            printf("Error in running command\n");
        }
        else
        {
            printf("%s", buffer);
            if (strcmp(buffer, "") != 0)
            {
                printf("\n");
            }
        }
    }
    close(sock);
}