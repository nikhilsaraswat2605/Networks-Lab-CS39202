#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <errno.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <poll.h>

#define MAXDATASIZE 1024
#define CHUNK_SIZE 1024

// function to send the output to the client in packets
void send_output(int sockfd, char *output, int len)
{
    int x = 0;
    while (1)
    {
        int mini = CHUNK_SIZE; // send the output in packets of CHUNK_SIZE bytes
        if (len - x < CHUNK_SIZE)
        {
            mini = len - x; // if the output is less than CHUNK_SIZE bytes, send the remaining bytes
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

// tokenize the request
char **tokenize(char *request, char *delim)
{
    char **tokens = malloc(1024 * sizeof(char *));
    char *token = strtok(request, delim);
    int i = 0;
    while (token != NULL)
    {
        tokens[i] = token;
        i++;
        token = strtok(NULL, delim);
    }
    tokens[i] = NULL;
    return tokens;
}

void get_IP_PORT(char *url, char *ip, int *port)
{
    // assume that the url is of the form http://10.98.78.2/docs/a1.pdf:8080, here 8080 is the port number and 10.98.78.2 is the IP address
    // keep copy of the url
    char *url_copy = malloc(1024 * sizeof(char));
    strcpy(url_copy, url);

    // get the IP address
    char *token = strtok(url, "//");
    if (token == NULL)
    {
        printf("Invalid URL.\n");
        if (url_copy)
            free(url_copy);
        if (token)
            free(token);
        exit(1);
    }

    token = strtok(NULL, "/");
    if (token == NULL)
    {
        printf("Invalid URL.\n");
        if (url_copy)
            free(url_copy);
        if (token)
            free(token);
        exit(1);
    }

    strcpy(ip, token);
    // get the port number
    token = strtok(NULL, ":");
    if (token == NULL)
    {
        printf("Invalid URL.\n");
        if (url_copy)
            free(url_copy);
        if (token)
            free(token);
        exit(1);
    }

    token = strtok(NULL, ":");
    if (token == NULL)
    {
        *port = 80;
    }
    else
    {
        *port = atoi(token);
    }

    // get the url back
    strcpy(url, url_copy);
    free(url_copy);
}

void get_file_extension(char *file_extension, char *file_path)
{
    // store file_path
    char *file_path_copy = malloc(1024 * sizeof(char));
    strcpy(file_path_copy, file_path);
    char *token = strtok(file_path_copy, ".");
    while (token != NULL)
    {
        strcpy(file_extension, token);
        token = strtok(NULL, ".");
    }
    // get the file_path back
    free(file_path_copy);
}

int get_file_size(char *file_path)
{
    FILE *file = fopen(file_path, "r");
    int size;

    fseek(file, 0, SEEK_END); // seek to end of file
    size = ftell(file);       // get current file pointer
    fseek(file, 0, SEEK_SET); // seek back to beginning of file
    fclose(file);

    return size;
}

void get_request(char *command, char *ip, int port)
{
    int sockfd, n = 0;
    char recvBuff[MAXDATASIZE];
    struct sockaddr_in serv_addr;

    memset(recvBuff, '0', sizeof(recvBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    inet_aton(ip, &serv_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        return;
    }

    char request[2048];
    char *token = strtok(command, "/");
    token = strtok(NULL, "/");
    token = strtok(NULL, ":");
    char *file_path = token;

    time_t t = time(NULL);
    struct tm *timeinfo = gmtime(&t);
    char buffer[80];
    strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
    // get time before two days
    time_t t2 = time(NULL) - 2 * 24 * 60 * 60;
    struct tm *timeinfo2 = gmtime(&t2);
    char buffer2[80];
    strftime(buffer2, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo2);
    char *lang = "en-us";
    char file_extension[1024];
    get_file_extension(file_extension, file_path);
    char Accept_Type[1024];
    if (strcmp(file_extension, "pdf") == 0)
    {
        strcpy(Accept_Type, "application/pdf");
    }
    else if (strcmp(file_extension, "html") == 0)
    {
        strcpy(Accept_Type, "text/html");
    }
    else if (strcmp(file_extension, "jpg") == 0)
    {
        strcpy(Accept_Type, "image/jpeg");
    }
    else
    {
        strcpy(Accept_Type, "text/*");
    }

    sprintf(request,
            "GET /%s HTTP/1.1\r\nHost: %s\r\nAccept: %s\r\nAccept-Language: %s\r\nDate: %s\r\nIf-modified-since: %s\r\nConnection: close\r\n\r\n", file_path, ip, Accept_Type, lang, buffer, buffer2);
    printf("\nRequest:\n\n%s", request);
    send_output(sockfd, request, strlen(request) + 1);
    // read the file from the server and write it to the file with the same name and same file extension
    // assume that the url is of the form http://10.98.78.2/docs/a1.pdf:8080, here 8080 is the port number and 10.98.78.2 is the IP address and a1.pdf is the file name
    // get the file name

    // get the file name only and remove the directory path
    token = strtok(token, "/");
    while (token != NULL)
    {
        file_path = token;
        token = strtok(NULL, "/");
    }
    // printf("File path: %s \n", file_path);
    // // open the file
    FILE *file;

    // Receive HTTP response
    char response_buffer[1024];
    int total_bytes_received = 0;
    int bytes_received = 0;
    memset(response_buffer, 0, sizeof(response_buffer));
    struct pollfd pfd;
    pfd.fd = sockfd;
    pfd.events = POLLIN;
    int timeout = 3000;
    int poll_result = poll(&pfd, 1, timeout);
    if(poll_result < 0)
    {
        printf("Error in poll()\n");
        return;
    }
    else if (poll_result == 0)
    {
        printf("Timeout\n");
        return;
    }
    while ((bytes_received = recv(sockfd, response_buffer, sizeof(response_buffer), 0)) > 0)
    {
        total_bytes_received += bytes_received;
        response_buffer[bytes_received] = '\0';
        if (total_bytes_received == bytes_received)
        {
            // This is the first chunk of the response, so extract the response code and body
            char *tmp = (char *)malloc(1024 * sizeof(char));
            strcpy(tmp, response_buffer);

            char *status_code = strtok(tmp, "\r\n");
            printf("Status Message: %s\n", status_code);

            if (strcmp(status_code, "HTTP/1.1 200 OK") != 0)
            {
                if (strcmp(status_code, "HTTP/1.1 404 Not Found") == 0)
                {
                    printf("404 Not Found\n");
                    exit(1);
                }
                else if (strcmp(status_code, "HTTP/1.1 400 Bad Request") == 0)
                {
                    printf("400 Bad Request\n");
                    exit(1);
                }
                else if (strcmp(status_code, "HTTP/1.1 403 Forbidden") == 0)
                {
                    printf("403 Forbidden\n");
                    exit(1);
                }
                else
                {
                    printf("Unknown Error\n");
                    exit(1);
                }
            }
            if (tmp)
                free(tmp);

            char *response_code = strstr(response_buffer, "\r\n\r\n");

            // Create a file to save the response body
            file = fopen(file_path, "w");
            if (file)
            {
                // delete the file
                fclose(file);
                remove(file_path);
            }

            file = fopen(file_path, "w");

            if (file == NULL)
            {
                perror("Error creating file");
                exit(1);
            }

            // Write the first chunk of the response body to the file
            fwrite(response_code + 4, sizeof(char), strlen(response_code + 4), file);
        }
        else
        {
            // This is a subsequent chunk of the response, so write it to the file
            fwrite(response_buffer, sizeof(char), bytes_received, file);
        }
        memset(response_buffer, 0, sizeof(response_buffer));
    }

    // Close the file and socket
    fclose(file);
    close(sockfd);
    sleep(1);
    // Fork a new process to open file in default software
    int pid = fork();
    if (pid == 0)
    {
        // printf("%s\n", file_extension);
        if (strcmp(file_extension, "pdf") == 0)
        {
            // use adobe acrobat to open pdf
            // char *args[] = {"xdg-open", file_path, NULL};
            char *args[] = {"acroread", file_path, NULL};
            execvp(args[0], args);
        }
        else if (strcmp(file_extension, "html") == 0)
        {
            // use firefox to open html
            char *args[] = {"firefox", file_path, NULL};
            execvp(args[0], args);
        }
        else if (strcmp(file_extension, "jpg") == 0)
        {
            // use default image viewer to open jpg
            char *args[] = {"xdg-open", file_path, NULL};
            execvp(args[0], args);
        }
        else
        {
            // use gedit to open other files
            char *args[] = {"gedit", file_path, NULL};
            execvp(args[0], args);
        }
    }
    else
    {
        printf("File Downloaded successfully!\n");
    }

    return;
}

void put_request(char *command, char *ip, int port, char *file_path)
{

    int sockfd, n = 0;
    char recvBuff[MAXDATASIZE];
    struct sockaddr_in serv_addr;

    memset(recvBuff, '0', sizeof(recvBuff));
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Error : Could not create socket \n");
        return;
    }
    // Open requested file
    printf("File path: %s\n", file_path);
    int fd = open(file_path, O_RDONLY);
    if (fd < 0)
    {
        perror("Error opening file");
        close(sockfd);
        exit(1);
    }
    int file_size = get_file_size(file_path);

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = inet_addr(ip);

    if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\n Error : Connect Failed \n");
        return;
    }

    char request[2048];
    char *token = strtok(command, "/");
    token = strtok(NULL, "/");
    token = strtok(NULL, ":");
    char *directory = token;
    strcat(directory, "/");
    strcat(directory, file_path);
    printf("Directory: %s \n", directory);
    time_t t = time(NULL);
    struct tm *timeinfo = gmtime(&t);
    char buffer[80];
    strftime(buffer, 80, "%a, %d %b %Y %H:%M:%S GMT", timeinfo);
    char *lang = "en-us";
    char file_extension[1024];
    get_file_extension(file_extension, file_path);
    char Accept_Type[1024];
    if (strcmp(file_extension, "pdf") == 0)
    {
        strcpy(Accept_Type, "application/pdf");
    }
    else if (strcmp(file_extension, "html") == 0)
    {
        strcpy(Accept_Type, "text/html");
    }
    else if (strcmp(file_extension, "jpg") == 0)
    {
        strcpy(Accept_Type, "image/jpeg");
    }
    else
    {
        strcpy(Accept_Type, "text/*");
    }
    memset(request, 0, sizeof(request));
    sprintf(request,
            "PUT /%s HTTP/1.1\r\nHost: %s\r\nContent-Length: %d\r\nAccept: %s\r\nAccept-Language: %s\r\nDate: %s\r\nConnection: close\r\n\r\n", directory, ip, file_size, Accept_Type, lang, buffer);
    printf("\nRequest:\n\n%s\n", request);
    // send the request
    send_output(sockfd, request, strlen(request) + 1);

    sleep(1);
    int bytes_received;
    char response_buffer[1024];
    int total_bytes_received = 0;
    memset(response_buffer, 0, sizeof(response_buffer));
    struct pollfd fds;
    fds.fd = sockfd;
    fds.events = POLLIN;
    int ret = poll(&fds, 1, 1000);
    if(ret < 0)
    {
        printf("Error in poll()\n");
        return;
    }
    else if(ret == 0)
    {
        printf("Timeout\n");
        return;
    }

    while ((bytes_received = recv(sockfd, response_buffer, sizeof(response_buffer), 0)) > 0)
    {
        total_bytes_received += bytes_received;
        response_buffer[bytes_received] = '\0';
        if (total_bytes_received == bytes_received)
        {
            // This is the first chunk of the response, so extract the response code and body
            char *tmp = (char *)malloc(1024 * sizeof(char));
            strcpy(tmp, response_buffer);

            char *status_code = strtok(tmp, "\r\n");
            printf("%s\n", status_code);

            if (strcmp(status_code, "HTTP/1.1 200 OK") != 0)
            {
                if (strcmp(status_code, "HTTP/1.1 404 Not Found") == 0)
                {
                    printf("404 Not Found\n");
                    exit(1);
                }
                else if (strcmp(status_code, "HTTP/1.1 400 Bad Request") == 0)
                {
                    printf("400 Bad Request\n");
                    exit(1);
                }
                else if (strcmp(status_code, "HTTP/1.1 403 Forbidden") == 0)
                {
                    printf("403 Forbidden\n");
                    exit(1);
                }
                else
                {
                    printf("Unknown Error\n");
                    exit(1);
                }
            }
            if (tmp)
                free(tmp);

            char *response_code = strstr(response_buffer, "\r\n\r\n");

            // Create a file to save the response body
        }

        memset(response_buffer, 0, sizeof(response_buffer));
        break;
    }

    // Send file contents
    char buf[1024];
    memset(buf, 0, sizeof(buffer));
    while ((bytes_received = read(fd, buf, sizeof(buf))) > 0)
    {
        send_output(sockfd, buf, bytes_received);
        memset(buf, 0, sizeof(buf));
    }
    close(fd);
    close(sockfd);
    printf("File uploaded successfully!\n");
}

int main()
{
    while (1)
    {
        printf("MyOwnBrowser> ");
        char request[1024];
        // take input from user until new line
        scanf("%[^\n]%*c", request);
        // tokenize the request
        char **tokens = tokenize(request, " ");
        if (tokens == NULL || tokens[0] == NULL)
        {
            printf("Invalid request\n");
            return 0;
        }
        int num_tokens = 0;
        while (tokens[num_tokens] != NULL)
        {
            num_tokens++;
        }
        // check if the request is GET
        if (strcmp(tokens[0], "GET") == 0 && num_tokens == 2)
        {
            printf("GET request received\n");
            char *url = tokens[1];
            char ip[100];
            int port;
            get_IP_PORT(url, ip, &port);
            printf("IP: %s and PORT: %d\n", ip, port);
            get_request(url, ip, port);
        }
        else if (strcmp(tokens[0], "PUT") == 0 && num_tokens == 3)
        {
            printf("PUT request received\n");
            char *url = tokens[1];
            char ip[100];
            int port;
            get_IP_PORT(url, ip, &port);
            printf("IP: %s and PORT: %d\n", ip, port);
            put_request(url, ip, port, tokens[2]);
        }
        else if (strcmp(tokens[0], "QUIT") == 0 && num_tokens == 1)
        {
            printf("QUIT request received\n");
            exit(0);
        }
        else
        {
            printf("Invalid request\n");
        }
        free(tokens);
    }
    return 0;
}
