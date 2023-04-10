#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/*client process*/
#define PORT 5566
#define IP "127.0.0.1"

int main()
{
    // char *IP = "127.0.0.1";
    int sock;
    struct sockaddr_in addr;
    char buffer[1024];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror(" [-]Error in sock\n");
        exit(1);
    }
    printf(" [+]TCP Server socket created successfully\n");

    // set the server_addr struct with all zeros
    memset(&addr, '\0', sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = inet_addr(IP);

    // connect the socket to the server_addr
    int status = connect(sock, (struct sockaddr *)&addr, sizeof(addr));
    if (status < 0)
    {
        perror(" [-]Error in connect\n");
        exit(1);
    }
    printf(" [+]Connection successful to port %d and IP %s \r \n", PORT, IP);

    // clear the buffer
    memset(buffer, '\0', sizeof(buffer));
    // ask time from server
    strcpy(buffer, " What is date and time now?\n");
    send(sock, buffer, strlen(buffer), 0);
    // clear the buffer
    memset(buffer, '\0', sizeof(buffer));
    // read the message from server and copy it in buffer
    recv(sock, buffer, 1024, 0);
    printf(" [+]Data received: %s\r \n", buffer);
    close(sock);
}