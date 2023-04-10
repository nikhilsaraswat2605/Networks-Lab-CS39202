/*
 * NAME: NIKHIL SARASWAT
 * ROLL NO.: 20CS10039
 * ASSIGNMENT NO.: 2
 * PROBLEM: 2
 * FILE NAME: sh_server.c
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>
#include <dirent.h>

#ifndef PORT // if port is not defined, define it to 5566
#define PORT 20000
#endif
#define CHUNK_SIZE 45 // maximum size of the data that can be sent in one go
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
// function to send the output to the client in packets
void send_dir(int sock, char *output, int len)
{
    int x = 0;
    while (1)
    {
        int mini = CHUNK_SIZE;    // send the output in packets of CHUNK_SIZE bytes
        if (len - x < CHUNK_SIZE) // if the output is less than CHUNK_SIZE bytes, send the remaining bytes
        {
            mini = len - x;
        }
        if (mini <= 0)
        {
            break;
        }
        int n = send(sock, output + x, mini, 0); // send the output to the client
        if (n < 0)
        {
            perror("[-]Error in send\n");
            exit(1);
        }
        x += n;
        if (x >= len - 1) // if the output is less than CHUNK_SIZE bytes, send the remaining bytes
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

int username_validation(char *username)
{
    FILE *fp = fopen("users.txt", "r"); // open the file containing the list of usernames
    char *line = NULL;                  // to store the username from the file
    size_t len = 0;                     // to store the length of the username
    while (getline(&line, &len, fp) != -1)
    {
        int n = strlen(line); // remove the newline character from the end of the username
        if(line[(int)strlen(line) - 1] == '\n')
        {
            line[(int)strlen(line) - 1] = '\0';
        }
        line[n] = '\0';       // compare the username with the username from the file
        if (strcmp(username, line) == 0)
        {
            fclose(fp); // if the username is valid, return 1
            return 1;
        }
    }
    fclose(fp); // if the username is not valid, return 0
    return 0;
}

int check_command_validity(const char *str, char *dirpath)
{
    // since cd is a shell command, we need to check if the command is cd or not, if it is cd then it must be followed by a space and then the directory name
    int i = 0;
    dirpath[0] = '\0';
    while (str[i] != '\0' && str[i] == ' ') // remove spaces from the beginning of the command
    {
        i++;
    }
    // cd is a shell command
    if (str[i] == 'c' && str[i + 1] == 'd' && (str[i + 2] == ' ' || str[i + 2] == '\0'))
    {
        // separate the directory name from the command
        int j = i + 2;
        while (str[j] != '\0' && str[j] == ' ')
        {
            j++;
        }
        int k = 0;
        while (str[j] != '\0' && str[j] != ' ')
        {
            dirpath[k] = str[j];
            j++;
            k++;
        }
        dirpath[k] = '\0';
        return 1;
    }
    // pwd is also a shell command
    else if (str[i] == 'p' && str[i + 1] == 'w' && str[i + 2] == 'd' && (str[i + 3] == ' ' || str[i + 3] == '\0'))
    {
        return 2;
    }
    // dir is also a shell command
    else if (str[i] == 'd' && str[i + 1] == 'i' && str[i + 2] == 'r' && (str[i + 3] == ' ' || str[i + 3] == '\0'))
    {
        int j = i + 3;
        while (str[j] != '\0' && str[j] == ' ')
        {
            j++;
        }
        int k = 0;
        while (str[j] != '\0' && str[j] != ' ')
        {
            dirpath[k] = str[j];
            j++;
            k++;
        }
        dirpath[k] = '\0';
        return 3;
    }
    else
    {
        return 0;
    }
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
    int sockfd, newsockfd; /* Socket descriptors */
    socklen_t clilen;
    struct sockaddr_in cli_addr, serv_addr;

    char buf[1024];                                     /* We will use this buffer for communication */
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) /* Create socket */
    {
        printf("[-]Cannot create socket\n");
        exit(0);
    }
    printf("[+]Server Socket created successfully.\n");
    serv_addr.sin_family = AF_INET;         /* Internet domain */
    serv_addr.sin_port = htons(PORT);       /* The given port */
    inet_aton(IP,&(serv_addr.sin_addr));

    if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("[-]Unable to bind local address\n");
        exit(0);
    }

    listen(sockfd, 5);

    while (1)
    {
        clilen = sizeof(cli_addr);                                         /* Size of client address */
        newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen); /* Accept connection from client */

        if (newsockfd < 0)
        { /* If accept fails */
            printf("[-]Accept error\n");
            exit(0);
        }
        printf("[+]Client connected from port=%d and IP=%s\n", ntohs(cli_addr.sin_port), inet_ntoa(cli_addr.sin_addr)); // print the client's IP and port

        if (fork() == 0)
        {
            close(sockfd);

            strcpy(buf, "LOGIN:");         // send LOGIN: to the client
            send_output(newsockfd, buf);   // send the message to the client
            receive_input(newsockfd, buf); // receive the username from the client
            if (username_validation(buf))  // check if the username is valid
            {
                strcpy(buf, "FOUND");
                send_output(newsockfd, buf); // send FOUND to the client
                while (1)
                {
                    receive_input(newsockfd, buf); // receive the command from the client
                    remove_spaces(buf);
                    if (strcmp(buf, "exit") == 0)  // if the client sends exit, close the connection
                    {
                        printf("[-]Client disconnected from port=%d and IP=%s\n", ntohs(cli_addr.sin_port), inet_ntoa(cli_addr.sin_addr)); // print the client's IP and port
                        close(newsockfd);
                        exit(0);
                    }
                    char dirpath[1024];                               // to store the path of the current directory
                    int check = check_command_validity(buf, dirpath); // check if the command is valid
                    if (check > 0)
                    {
                        // if command is cd
                        if (check == 1)
                        {
                            // change directory
                            if (chdir(dirpath) == 0)
                            {
                                strcpy(buf, ""); // send an empty string to the client
                                send_output(newsockfd, buf);
                            }
                            else
                            {
                                strcpy(buf, "####"); // send #### to the client
                                send_output(newsockfd, buf);
                            }
                        }
                        // if command is pwd
                        else if (check == 2)
                        {
                            char *temp = getcwd(NULL, 0); // get the current directory
                            send_output(newsockfd, temp);
                            free(temp);
                        }
                        // if command is dir
                        else if (check == 3)
                        {
                            if (dirpath[0] == '\0')
                            {
                                getcwd(dirpath, sizeof(dirpath));
                            }
                            DIR *d;
                            struct dirent *dir;
                            d = opendir(dirpath); // open the directory
                            if (d)
                            {
                                // int idx = 0;
                                char temp[1024];
                                while ((dir = readdir(d)) != NULL) // read the directory
                                {

                                    // append files to temp
                                    strcpy(temp, dir->d_name); // copy the file name to temp
                                    int len = strlen(temp);
                                    temp[len] = '\n';
                                    send_dir(newsockfd, temp, len + 1); // send the file name to the client
                                }
                                closedir(d);
                                memset(temp, 0, sizeof(temp));
                                send_output(newsockfd, temp);
                            }
                            else
                            {
                                strcpy(buf, "####"); // if error in opening directory
                                send_output(newsockfd, buf);
                            }
                        }
                    }
                    else
                    {
                        strcpy(buf, "$$$$"); // if command is invalid
                        send_output(newsockfd, buf);
                    }
                }
            }
            else
            {
                strcpy(buf, "NOT-FOUND"); // if username is invalid
                send_output(newsockfd, buf);
            }
            close(newsockfd); // close the connection
            exit(1);          // exit the child process
        }

        close(newsockfd); // close the connection
    }
    return 0;
}
