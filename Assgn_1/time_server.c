#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

/*server process*/
#define PORT 5566
#define IP "127.0.0.1"

int main()
{
    int server_socket, client_socket;            // socket descriptors
    struct sockaddr_in server_addr, client_addr; // socket address structures
    socklen_t clilen;                            // socket length
    char buffer[1024];                           // buffer to read and write data

    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) // create a socket
    {
        perror(" [-]Error in socket\n");
        exit(1);
    }
    printf(" [+]TCP Server socket created successfully\n");

    // set the server_addr struct with all zeros
    memset(&server_addr, '\0', sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = inet_addr(IP);
    // bind the socket to the server_addr
    int status = bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)); // bind to server
    if (status < 0)
    {
        perror(" [-]Error in bind\n");
        exit(1);
    }
    printf(" [+]Binding successful to port %d and IP %s \r \n", PORT, IP);
    listen(server_socket, 5); // listen for connections

    while (1)
    {
        printf("\nListening for incoming connections...\r \n");
        clilen = sizeof(client_addr);
        client_socket = accept(server_socket, (struct sockaddr *)&client_addr, &clilen); // accept connection from client
        if (client_socket < 0)
        {
            perror(" [-]Error in accept\n");
            exit(1);
        }
        printf(" [+]Connection accepted from %s:%d\r \n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
        // clear the buffer
        memset(buffer, '\0', sizeof(buffer));
        // read the message from client and copy it in buffer
        recv(client_socket, buffer, 1024, 0); // receive from client
        printf(" [+]Data received: %s\r \n", buffer);
        // current time
        time_t t = time(NULL);         // get current time
        struct tm tm = *localtime(&t); // convert to local time
        // send the message to client
        send(client_socket, asctime(&tm), strlen(asctime(&tm)), 0); // send to client
        close(client_socket);
        printf(" [+]Connection closed\r \n");
    }
}