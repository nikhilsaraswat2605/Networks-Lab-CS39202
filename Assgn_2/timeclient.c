// A Simple Client Implementation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>

int main()
{
    int sockfd;
    struct sockaddr_in servaddr;

    // Creating socket file descriptor
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
    {
        perror("[-]socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&servaddr, 0, sizeof(servaddr));

    // Server information
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(8181);
    servaddr.sin_addr.s_addr = INADDR_ANY;

    int n;
    socklen_t len;
    // char *hello = "CLIENT: Give me time and date...";

    // sendto(sockfd, (const char *)hello, strlen(hello), 0,
    //        (const struct sockaddr *)&servaddr, sizeof(servaddr));
    char buffer[1024];
    len = sizeof(servaddr);
    int ret = 0;
    // wait for 3 seconds for a response from the server and try 5 times then timeout using poll
    for (int i = 0; i < 5; i++)
    {
        struct pollfd fds;
        fds.fd = sockfd;
        fds.events = POLLIN;
        char *msg = "CLIENT: Give me time and date...";
        sendto(sockfd, (const char *)msg, strlen(msg), 0,
        (const struct sockaddr *)&servaddr, sizeof(servaddr));
        ret = poll(&fds, 1, 3000);
        if (ret > 0)
        {
            n = recvfrom(sockfd, (char *)buffer, 1024, 0,
                         (struct sockaddr *)&servaddr, &len);
            buffer[n] = '\0';
            printf("Time and Date: %s", buffer);
            break;
        }
        else
        {
            printf("[-]No response from server, trying again...\n");
        }
    }
    if (ret == 0)
    {
        printf("Timeout Exceeded.\n");
    }
    close(sockfd);
    return 0;
}