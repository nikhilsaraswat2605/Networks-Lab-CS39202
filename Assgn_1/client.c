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


char *input()
{
    char *str = (char *)malloc(sizeof(char)); // allocate memory for a si
    if (str == NULL)                          // check if the memory is allocated successfully
    {
        perror("[-]Error in malloc");
        exit(1);
    }
    str[0] = '\0'; // initialize the first byte with null character
    char c;
    int i = 0;
    while ((c = getchar()) != '\n') // read the input from the user until the user presses enter
    {
        str[i] = c; // store the character in the string
        i++;
        str = (char *)realloc(str, (i + 1) * sizeof(char)); // reallocate memory for the next character
        if (str == NULL)                                    // check if the memory is allocated successfully
        {
            perror("[-]Error in realloc");
            exit(1);
        }
        str[i] = '\0'; // add null character at the end of the string
    }
    return str; // return the string
}

int main()
{
    // char *IP = "127.0.0.1";
    int sockfd;
    struct sockaddr_in serv_addr;
    char buffer[1024];

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // create a socket
    {
        perror(" [-]Error in sockfd\n");
        exit(1);
    }
    printf(" [+]TCP Server socket created successfully\n"); // print a message if the socket is created successfully

    serv_addr.sin_family = AF_INET;
    inet_aton(IP, &serv_addr.sin_addr);
    serv_addr.sin_port = htons(PORT);

    // connect the socket to the server_addr
    int status;
    if ((status = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))) < 0) // connect the socket to the server_addr
    {
        perror(" [-]Error in connect\n");
        exit(1);
    }
    printf(" [+]Connection successful to port %d and IP %s \r \n", PORT, IP); // print a message if the connection is successful

    // ask user to enter an arithmetic expression to be evaluated by the server of unkwown length
    while (1)
    {
        printf(" [+]Enter an arithmetic expression to be evaluated by the server: ");
        char *str = NULL;
        size_t len = 0;
        getline(&str, &len, stdin);
        len = (int)strlen(str);
        char *quit = "q";
        if (len == 3 && str[0] == '-' && str[1] == '1')
        {
            printf(" [+]Terminating the connection \r \n");
            send(sockfd, quit, 5, 0);
            break;
        }
        printf(" [+]Sending the expression to the server: %s \r \n", str);
        // sending the expression
        int x = 0;
        while (1)
        {
            int n = send(sockfd, str + x, len + 1 - x, 0); // send the expression to the server
            if (n == -1)
            {
                perror(" [-]Error in sending the expression \r \n");
                exit(1);
            }
            x += n;
            printf(" [+]Sent %d bytes \r \n", x);
            if (x >= len)
            {
                break;
            }
            // break;
        }
        printf(" [+]Expression sent successfully \r \n");
        free(str); // free the memory allocated for the string
        // clear the buffer
        memset(buffer, '\0', sizeof(buffer));
        // receive the result from the server
        recv(sockfd, buffer, sizeof(buffer), 0);
        printf(" [+]Result received from the server: %s \r \n\n", buffer);
    }

    close(sockfd);
}