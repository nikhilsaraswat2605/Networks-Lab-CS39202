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
#define CHUNK_SIZE 1024

int div_zero_flag=0;

double evaluate_expr(char *expr, int size)
{
    int i = 0;
    double result = 0;
    double operand = 0;
    char operator= '+';
    while (i < size)
    {
        if (expr[i] == ' ' || expr[i] == '\t')
        {
            i++;
            continue;
        }
        else if (expr[i] >= '0' && expr[i] <= '9') // if the character is a digit
        {
            operand = operand * 10 + (expr[i] - '0');
        }
        else if (expr[i] == '.')
        {
            int j = i + 1;
            double decimal = 0;
            int count = 0;
            while (expr[j] >= '0' && expr[j] <= '9')
            {
                decimal = decimal * 10 + (expr[j] - '0');
                count++;
                j++;
            }
            i = j - 1;
            while (count > 0)
            {
                decimal = decimal / 10;
                count--;
            }
            operand = operand + decimal;
        }
        else if (expr[i] == '(') // if the character is an opening bracket
        {
            int j = i;
            int count = 0;
            while (j < size)
            {
                if (expr[j] == '(')
                {
                    count++;
                }
                else if (expr[j] == ')')
                {
                    count--;
                }
                if (count == 0)
                {
                    break;
                }
                j++;
            }
            operand = evaluate_expr(expr + i + 1, j - i - 1); // evaluate the expression inside the brackets
            i = j;
        }
        else
        {
            if (operator== '+') // if the operator is addition
            {
                result += operand;
            }
            else if (operator== '-') // if the operator is subtraction
            {
                result -= operand;
            }
            else if (operator== '*') // if the operator is multiplication
            {
                result *= operand;
            }
            else if (operator== '/') // if the operator is division
            {
                if (abs(operand) < (1e-9))
                {
                    printf(" [-]Error in division by zero\n");
                    // return maximum value of double
                    div_zero_flag=1;
                    return 1e18;
                }
                result /= operand;
            }
            operator= expr[i]; // store the current operator
            operand = 0;       // reset the operand
        }
        i++;
    }
    if (operator== '+')
    {
        result += operand;
    }
    else if (operator== '-')
    {
        result -= operand;
    }
    else if (operator== '*')
    {
        result *= operand;
    }
    else if (operator== '/')
    {
        if (abs(operand) < (1e-9))
        {
            printf(" [-]Error in division by zero\n");
            // return maximum value of double
            div_zero_flag=1;
            return 1e18;
        }
        result /= operand;
    }
    return result; // return the result
}

int main()
{
    int sockfd, newsockfd;                       // socket descriptors
    struct sockaddr_in server_addr, client_addr; // socket address structures
    socklen_t clilen;                                  // size of the socket address
    char buffer[1024];                           // buffer to read and write data

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) // create a socket
    {
        perror(" [-]Error in socket\n");
        exit(1);
    }
    printf(" [+]TCP Server socket created successfully\n"); // socket created successfully

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;
    // bind the socket to the server_addr
    int status = bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)); // bind the socket to the server address
    if (status < 0)
    {
        perror(" [-]Error in bind\n");
        exit(1);
    }
    printf(" [+]Binding successful to port %d and IP %s \r \n", PORT, IP);
    listen(sockfd, 5); // listen for incoming connections

    while (1)
    {
        printf("\n   Listening for incoming connections...\r \n");
        clilen = sizeof(client_addr);                                         // size of the client address
        newsockfd = accept(sockfd, (struct sockaddr *)&client_addr, &clilen); // accept the connection
        if (newsockfd < 0)
        {
            perror(" [-]Error in accept\n");
            exit(1);
        }
        printf(" [+]Connection accepted from %s:%d\r \n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));

        while (1)
        {
            char *expr = (char *)malloc(CHUNK_SIZE * sizeof(char)); // allocate memory for the expression
            // read the expression from client  character by character and store it in expr
            int i = 0;
            int terminate = 0;

            while ((status = recv(newsockfd, buffer, CHUNK_SIZE, 0)) > 0) // read the expression CHUNK_SIZE character chunk at a time
            {
                int len = strlen(buffer);
                if (len == 1 && buffer[0] == 'q')
                {
                    printf(" [+]Client disconnected\r \n");
                    terminate = 1;
                    break;
                }
                int j = 0;    // index to read the buffer
                int flag = 0; // flag to check if the expression is complete
                while (j < CHUNK_SIZE)
                {
                    if (buffer[j] == '\0')
                    {
                        flag = 1;
                        // clear the buffer
                        memset(buffer, '\0', sizeof(buffer));
                        break;
                    }
                    expr[i] = buffer[j];
                    i++;
                    j++;
                }
                expr = (char *)realloc(expr, (i + CHUNK_SIZE) * sizeof(char)); // reallocate memory for the expression
                expr[i] = '\0';
                if (flag == 1)
                    break;
            }
            if (terminate)
            {
                break;
            }
            printf(" [+]expr received: %s\r \n", expr);
            int size = strlen(expr) + 1;
            double result = evaluate_expr(expr, size);
            // clear the buffer
            memset(buffer, '\0', sizeof(buffer));
            if (div_zero_flag==1)
            {
                div_zero_flag=0;
                strcpy(buffer, " [-]Error in division by zero");
                send(newsockfd, buffer, strlen(buffer), 0);
                free(expr);
                continue;
            }
            // copy the contents of result to buffer
            sprintf(buffer, "%lf", result);
            // and send that buffer to client
            send(newsockfd, buffer, strlen(buffer), 0);
            free(expr); // free the memory allocated for the expression
        }
        close(newsockfd);
    }
}