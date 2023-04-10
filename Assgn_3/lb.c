#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>
#include <unistd.h>
#include <sys/time.h>

// load balancer code

#define IP "127.0.0.1"
#define CHUNK_SIZE 10

long long timeInMilliseconds(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (((long long)tv.tv_sec) * 1000) + (tv.tv_usec / 1000);
}

// function to send the output to the client in packets
void send_output(int sockfd, char *output)
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
        int n = send(sockfd, output + x, mini, 0); // send the output to the client
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
void receive_input(int sockfd, char *input)
{

    memset(input, 0, (size_t)sizeof(input));
    int i = 0;
    while (1)
    {
        char buf[CHUNK_SIZE];
        memset(buf, 0, CHUNK_SIZE);
        int bytesReceived = recv(sockfd, buf, CHUNK_SIZE, 0);
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
    int sockfd, S1sockfd, S2sockfd;
    struct sockaddr_in load_balancerAddr, s1Addr, s2Addr;
    int lbport, s1Port, s2Port;
    char buffer[1024];
    int ServerLoad[2] = {0, 0};
    struct pollfd fds;
    int timeout = 5000; // 5 seconds
    int startTime;

    if (argc != 4)
    {
        printf("Missing arguments\n");
        exit(1);
    }
    lbport = atoi(argv[1]);
    s1Port = atoi(argv[2]);
    s2Port = atoi(argv[3]);

    // create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        printf("[-]Could not create socket\n");
        exit(1);
    }

    // prepare the sockaddr_in structure
    load_balancerAddr.sin_family = AF_INET;
    load_balancerAddr.sin_port = htons(lbport);
    inet_aton(IP, &load_balancerAddr.sin_addr);

    // bind socket
    if (bind(sockfd, (struct sockaddr *)&load_balancerAddr, sizeof(load_balancerAddr)) < 0)
    {
        printf("[-]Bind failed\n");
        exit(1);
    }

    printf("[+]Load balancer started on IP %s and port %d\n", IP, lbport);
    // listen on socket
    listen(sockfd, 5);

    // prepare the sockaddr_in structure
    s1Addr.sin_family = AF_INET;
    s1Addr.sin_addr.s_addr = inet_addr(IP);
    s1Addr.sin_port = htons(s1Port);

    // prepare the sockaddr_in structure
    s2Addr.sin_family = AF_INET;
    s2Addr.sin_addr.s_addr = inet_addr(IP);
    s2Addr.sin_port = htons(s2Port);

    // set up poll
    fds.fd = sockfd;
    fds.events = POLLIN;
    S1sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (S1sockfd == -1)
    {
        printf("[-]Could not create socket\n");
        exit(1);
    }
    if (connect(S1sockfd, (struct sockaddr *)&s1Addr, sizeof(s1Addr)) < 0)
    {
        printf("[-]Connect failed to server 1\n");
        close(S1sockfd);
        exit(1);
    }
    strcpy(buffer, "Send Load");
    send_output(S1sockfd, buffer);
    // receive load

    receive_input(S1sockfd, buffer);
    ServerLoad[0] = atoi(buffer);
    close(S1sockfd);
    // print IP of load_balancerAddr with load
    printf("Load received from IP %s and Port %d: %d\n", inet_ntoa(s1Addr.sin_addr), ntohs(s1Addr.sin_port), ServerLoad[0]);

    S2sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (S2sockfd == -1)
    {
        printf("[-]Could not create socket\n");
        exit(1);
    }
    if (connect(S2sockfd, (struct sockaddr *)&s2Addr, sizeof(s2Addr)) < 0)
    {
        printf("[-]Connect failed to server 2\n");
        close(S2sockfd);
        exit(1);
    }
    strcpy(buffer, "Send Load");

    send_output(S2sockfd, buffer);
    // receive load

    receive_input(S2sockfd, buffer);
    ServerLoad[1] = atoi(buffer);
    close(S2sockfd);

    printf("Load received from IP %s and Port %d: %d\n", inet_ntoa(s2Addr.sin_addr), ntohs(s2Addr.sin_port), ServerLoad[1]);

    // printf("Timeout: %d\n", timeout);
    timeout = 5000;
    while (1)
    {
        // poll
        startTime = timeInMilliseconds();
        int ret = poll(&fds, 1, timeout);
        if (ret == -1)
        {
            printf("[-]Poll failed\n");
            exit(1);
        }
        else if (ret == 0)
        {
            // timeout, check load
            S1sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (S1sockfd == -1)
            {
                printf("[-]Could not create socket\n");
                exit(1);
            }
            if (connect(S1sockfd, (struct sockaddr *)&s1Addr, sizeof(s1Addr)) < 0)
            {
                printf("[-]Connect failed to server 1\n");
                close(S1sockfd);
                exit(1);
            }
            strcpy(buffer, "Send Load");
            send_output(S1sockfd, buffer);
            // receive load

            receive_input(S1sockfd, buffer);
            ServerLoad[0] = atoi(buffer);
            close(S1sockfd);
            // print IP of load_balancerAddr with load
            printf("Load received from IP %s and Port %d: %d\n", inet_ntoa(s1Addr.sin_addr), ntohs(s1Addr.sin_port), ServerLoad[0]);

            S2sockfd = socket(AF_INET, SOCK_STREAM, 0);
            if (S2sockfd == -1)
            {
                printf("[-]Could not create socket\n");
                exit(1);
            }
            if (connect(S2sockfd, (struct sockaddr *)&s2Addr, sizeof(s2Addr)) < 0)
            {
                printf("[-]Connect failed to server 2\n");
                close(S2sockfd);
                exit(1);
            }
            strcpy(buffer, "Send Load");

            send_output(S2sockfd, buffer);
            // receive load

            receive_input(S2sockfd, buffer);
            ServerLoad[1] = atoi(buffer);
            close(S2sockfd);

            printf("Load received from IP %s and Port %d: %d\n", inet_ntoa(s2Addr.sin_addr), ntohs(s2Addr.sin_port), ServerLoad[1]);

            // printf("Timeout: %d\n", timeout);
            timeout = 5000;
        }
        else
        {
            if (fds.revents & POLLIN)
            {
                // accept connection
                int clilen = sizeof(struct sockaddr_in);
                struct sockaddr_in client_addr;
                int clientsockfd = accept(sockfd, (struct sockaddr *)&client_addr, (socklen_t *)&clilen);
                if (clientsockfd < 0)
                {
                    printf("[-]Accept failed\n");
                    continue;
                }
                printf("[+]Connection accepted from %s and port %d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
                // fork
                int pid = fork();
                if (pid < 0)
                {
                    printf("[-]Fork failed\n");
                    continue;
                }
                else if (pid == 0)
                {
                    // child process
                    // receive message
                    close(sockfd);
                    // check min load and send to that load_balancerAddr
                    int serverSock;
                    int servernum = 1;
                    if (ServerLoad[0] < ServerLoad[1])
                    {
                        serverSock = socket(AF_INET, SOCK_STREAM, 0);
                        if (serverSock == -1)
                        {
                            printf("[-]Could not create socket\n");
                            exit(1);
                        }
                        if (connect(serverSock, (struct sockaddr *)&s1Addr, sizeof(s1Addr)) < 0)
                        {
                            printf("[-]Connect failed\n");
                            close(serverSock);
                            exit(1);
                        }
                        servernum = 1;
                    }
                    else
                    {
                        serverSock = socket(AF_INET, SOCK_STREAM, 0);
                        if (serverSock == -1)
                        {
                            printf("[-]Could not create socket\n");
                            exit(1);
                        }
                        if (connect(serverSock, (struct sockaddr *)&s2Addr, sizeof(s2Addr)) < 0)
                        {
                            printf("[-]Connect failed\n");
                            close(serverSock);
                            exit(1);
                        }
                        servernum = 2;
                    }
                    // receive message from client

                    receive_input(clientsockfd, buffer);
                    // send message to load_balancerAddr
                    if (servernum == 1)
                    {
                        printf("Sending client request to IP %s and Port %d\n", inet_ntoa(s1Addr.sin_addr), ntohs(s1Addr.sin_port));
                    }
                    else
                    {
                        printf("Sending client request to IP %s and Port %d\n", inet_ntoa(s2Addr.sin_addr), ntohs(s2Addr.sin_port));
                    }
                    send_output(serverSock, buffer);
                    // receive message from load_balancerAddr

                    receive_input(serverSock, buffer);
                    // send message to client
                    send_output(clientsockfd, buffer);
                    // close sockets
                    close(clientsockfd);
                    close(serverSock);
                    exit(0);
                }
                else
                {
                    // parent
                    close(clientsockfd);
                    int endTime = timeInMilliseconds();
                    timeout -= (endTime - startTime);
                    if (timeout < 0)
                    {
                        timeout = 0;
                    }
                    // printf("Timeout: %d\n", timeout);
                }
            }
        }
    }
    return 0;
}