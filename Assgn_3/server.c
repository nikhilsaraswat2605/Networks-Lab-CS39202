#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>

#define IP "127.0.0.1"
#define CHUNK_SIZE 10

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
    if (argc < 2)
    {
        printf("Missing arguments!\n");
        exit(1);
    }

    // get port number
    int port = atoi(argv[1]);

    // create server socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        perror("[-]Error creating socket");
        exit(1);
    }

    // bind socket to port
    struct sockaddr_in addr, client_addr;
    int clilen;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    // addr.sin_addr.s_addr = inet_addr(IP);
    inet_aton(IP, &addr.sin_addr);

    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("Error binding socket to port");
        close(sock);
        exit(1);
    }

    printf("[+]Server started on IP %s and port %d\n", IP, port);

    // listen for incoming connections
    if (listen(sock, 5) < 0)
    {
        perror("Error listening for incoming connections");
        close(sock);
        exit(1);
    }

    // seed random number generator
    srand(port);

    while (1)
    {
        // accept incoming connection

        int clientSock = accept(sock, (struct sockaddr *)&client_addr, (socklen_t *)&clilen);
        if (clientSock < 0)
        {
            perror("Error accepting incoming connection");
            continue;
        }

        // receive request from client
        char buffer[1024];
        receive_input(clientSock, buffer);

        // check if request is for load or service
        if (strcmp(buffer, "Send Load") == 0)
        {
            // generate random load
            int load = rand() % 100 + 1;
            // convert load to string
            sprintf(buffer, "%d", load);

            // send load to client
            send_output(clientSock, buffer);

            // print message indicating load sent
            printf("Load sent: %d\n", load);
        }
        else if (strcmp(buffer, "Send Time") == 0)
        {
            // get current time
            time_t currentTime = time(NULL);
            char *timeString = ctime(&currentTime);

            // send time to client
            send_output(clientSock, timeString);
        }
        else
        {
            printf("Invalid request received from client\n");
        }

        // close client socket
        close(clientSock);
    }
    close(sock);
    return 0;
}
