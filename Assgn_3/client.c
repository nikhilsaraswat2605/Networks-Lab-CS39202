#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>

#define CHUNK_SIZE 10
#define IP "127.0.0.1"

// function to send the output to the client in packets
void send_output(int sock, char *output)
{
    int len = strlen(output); //
    int x = 0;
    while (1)
    {
        int mini = CHUNK_SIZE; // send the output in packets of CHUNK_SIZE bytes
        if (len + 1 - x < CHUNK_SIZE)
        {
            mini = len + 1 - x; // if the output is less than CHUNK_SIZE bytes, send the remaining bytes
        }
        int n = send(sock, output + x, mini, 0); // send the output to the client
        if (n < 0)
        {
            perror(" [-]Error in send\n"); // if error in sending
            exit(1);
        }
        x += n;
        if (x >= len)
        {
            break;
        }
    }
}

// function to receive the input from the client in CHUNK_SIZE bytes
void receive_input(int sock, char *input)
{

    memset(input, 0, (size_t)sizeof(input));
    int i = 0;
    while (1)
    {
        char buf[CHUNK_SIZE];
        memset(buf, 0, CHUNK_SIZE);
        int bytesReceived = recv(sock, buf, CHUNK_SIZE, 0);
        if (bytesReceived < 0)
        {
            perror(" [-]Error in recv\n");
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
        }
        if (buf[bytesReceived - 1] == '\0')
        {
            break;
        }
    }
    input[i] = '\0';
}

int main(int argc, char *argv[])
{
    if (argc < 1)
    {
        printf("Missing arguments\n");
        exit(1);
    }

    // get port number from command line
    int lbPort = atoi(argv[1]);

    // create client socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("Error creating socket");
        exit(1);
    }

    // connect to load balancer
    struct sockaddr_in lbAddr;

    lbAddr.sin_family = AF_INET;
    lbAddr.sin_port = htons(lbPort);
    inet_aton(IP, &lbAddr.sin_addr);

    if (connect(sock, (struct sockaddr *)&lbAddr, sizeof(lbAddr)) < 0)
    {
        perror("Error connecting to load balancer");
        close(sock);
        exit(1);
    }
    char buffer[1024];
    strcpy(buffer, "Send Time");

    // send own port to load balancer
    send_output(sock, buffer);

    // receive time from load balancer
    receive_input(sock, buffer);

    // print received time
    printf("Time received: %s", buffer);

    // close client socket
    close(sock);

    return 0;
}
