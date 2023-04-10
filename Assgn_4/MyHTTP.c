#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>

#define SERVER_PORT 8080
#define SUCCESS_RESPONSE "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n"
#define NOT_FOUND_RESPONSE "HTTP/1.1 404 Not Found\r\nContent-Type: text/html\r\n\r\n"
#define BAD_REQUEST_RESPONSE "HTTP/1.1 400 Bad Request\r\nContent-Type: text/html\r\n\r\n"
#define FORBIDDEN_RESPONSE "HTTP/1.1 403 Forbidden\r\nContent-Type: text/html\r\n\r\n"
#define NOT_MODIFIED_RESPONSE "HTTP/1.1 304 Not Modified\r\nContent-Type: text/html\r\n\r\n"
#define CHUNK_SIZE 1024

char **tokenize(char *request, int *num_tokens, char *delim)
{
    char **tokens = malloc(20 * sizeof(char *));
    // make copy of request string to avoid modifying it
    char *request_copy = malloc(strlen(request) + 1);
    memset(request_copy, 0, strlen(request) + 1);
    strcpy(request_copy, request);
    // tokenize request string
    char *token = strtok(request_copy, delim);
    int i = 0;
    while (token != NULL)
    {
        tokens[i] = (char *)malloc(sizeof(char) * (strlen(token) + 1)); // allocate memory for each token
        strcpy(tokens[i], token);
        token = strtok(NULL, delim);
        i++;
    }
    *num_tokens = i;
    free(request_copy);
    return tokens;
}

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

void to_lower(char *str)
{
    for (int i = 0; i < strlen(str); i++)
    {
        if (str[i] >= 'A' && str[i] <= 'Z')
        {
            str[i] = str[i] + 32;
        }
    }
}
int compare_last_modification_time(const char *filename, const char *time_str)
{
    struct stat st;
    if (stat(filename, &st) != 0)
    {
        printf("Failed to get file information.\n");
        return -1;
    }

    struct tm time_info; 
    memset(&time_info, 0, sizeof(time_info));
    // Parse time string and convert it to struct tm
    if (strptime(time_str, " %a, %d %b %Y %H:%M:%S %Z", &time_info) == NULL)
    {
        printf("%s\n", time_str);
        printf("Failed to parse time string.\n");
        return -1;
    }
    time_t time_value = mktime(&time_info);
    printf("time_value: %ld\n", time_value);
    printf("st.st_mtime: %ld\n", st.st_mtime);
    return (time_value <= st.st_mtime);
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

char *get_last_modification_date_time(char *file_path)
{
    struct stat st;
    if (stat(file_path, &st) != 0)
    {
        printf("Failed to get file information.\n");
        return NULL;
    }
    struct tm *time_info = gmtime(&st.st_mtime);
    char *time_str = malloc(100 * sizeof(char));
    strftime(time_str, 100, "%a, %d %b %Y %H:%M:%S %Z", time_info);
    return time_str;
}

char *get_content_type(char *file_path)
{
    char *content_type = malloc(100 * sizeof(char));
    if (strstr(file_path, ".html") != NULL)
    {
        strcpy(content_type, "text/html");
    }
    else if (strstr(file_path, ".jpg") != NULL)
    {
        strcpy(content_type, "image/jpeg");
    }
    else if (strstr(file_path, ".png") != NULL)
    {
        strcpy(content_type, "image/png");
    }
    else if (strstr(file_path, ".pdf") != NULL)
    {
        strcpy(content_type, "application/pdf");
    }
    else
    {
        strcpy(content_type, "text/*");
    }
    return content_type;
}

// MyOwnHTTP maintains an Access Log (AccessLog.txt) which records every client accesses. Format of
// every record/line of the AccessLog.txt: <Date(ddmmyy)>:<Time(hhmmss)>:<Client IP>:<Client
// Port>:<GET/PUT>:<URL>
void Access_log(char *ip, int port, char *method, char *url)
{
    FILE *fp = fopen("AccessLog.txt", "a");
    if (fp == NULL)
    {
        printf("Error opening file!\n");
        exit(1);
    }
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);
    // AccessLog.txt: <Date(ddmmyy)>:<Time(hhmmss)>:<Client IP>:<Client Port>:<GET/PUT>:<URL>
    fprintf(fp, "%02d%02d%02d:%02d%02d%02d:%s:%d:%s:%s\n", tm.tm_mday, tm.tm_mon + 1, tm.tm_year + 1900, tm.tm_hour, tm.tm_min, tm.tm_sec, ip, port, method, url);
    fclose(fp);
}

int main(int argc, char *argv[])
{
    int portno = SERVER_PORT;
    if (argc == 2)
    {
        portno = atoi(argv[1]);
    }
    int server_socket, client_socket, fd;
    struct sockaddr_in server_address, client_address;
    socklen_t client_address_size;
    char buffer[1024];

    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0)
    {
        perror("Error opening server socket");
        exit(1);
    }

    // Configure server address
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = INADDR_ANY;
    server_address.sin_port = htons(portno);

    // Bind server socket to address
    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
    {
        perror("Error binding server socket");
        exit(1);
    }

    // Listen for client connections
    if (listen(server_socket, 5) < 0)
    {
        perror("Error listening for client connections");
        exit(1);
    }

    printf("MyHTTP server running on port %d\n", ntohs(server_address.sin_port));

    while (1)
    {
        // Accept client connection
        client_address_size = sizeof(client_address);
        client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_address_size);
        if (client_socket < 0)
        {
            perror("Error accepting client connection");
            continue;
        }
        printf("Accepted client connection from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        pid_t pid = fork();
        // Read HTTP request
        if(pid == 0)
        {
            close(server_socket);
            int bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
            if (bytes_received < 0)
            {
                perror("Error receiving HTTP request");
                close(client_socket);
                exit(1);
            }
            int num_tokens;
            char **tokens = tokenize(buffer, &num_tokens, "\r\n");
            char *header_type = tokens[0];
            int cnt = 0;
            char **req_type_dir = tokenize(header_type, &cnt, " ");
            char *request_type = req_type_dir[0];
            char *file_path = req_type_dir[1];
            file_path++;

            if (strcmp(request_type, "GET") == 0)
            {

                // Open requested file
                fd = open(file_path, O_RDONLY);
                if (fd < 0)
                {
                    perror("Error opening file\n");
                    if (errno == 2)
                        send_output(client_socket, NOT_FOUND_RESPONSE, strlen(NOT_FOUND_RESPONSE));
                    else if (errno == 13)
                        send_output(client_socket, FORBIDDEN_RESPONSE, strlen(FORBIDDEN_RESPONSE));
                    else
                        send_output(client_socket, BAD_REQUEST_RESPONSE, strlen(BAD_REQUEST_RESPONSE));
                    close(client_socket);
                    exit(1);
                }
                int flag = 0;
                for (int i = 0; i < num_tokens; i++)
                {
                    int cnt = 0;
                    char *temp = strtok(tokens[i], ": ");

                    if (temp == NULL)
                        continue;
                    to_lower(temp);
                    if (strcmp(temp, "if-modified-since") == 0)
                    {
                        temp = strtok(NULL, "");
                        int x = compare_last_modification_time(file_path, temp);
                        if (x < 0)
                        {
                            send_output(client_socket, NOT_FOUND_RESPONSE, strlen(NOT_FOUND_RESPONSE));
                            close(client_socket);
                            flag = 1;
                            break;
                        }
                        else if (x == 0)
                        {
                            send_output(client_socket, NOT_MODIFIED_RESPONSE, strlen(NOT_MODIFIED_RESPONSE));
                            close(client_socket);
                            flag = 1;
                            break;
                        }
                    }
                }
                if (flag == 1)
                    exit(1);

                // Send HTTP response header
                char success_response[1024];
                memset(success_response, 0, sizeof(success_response));
                // expires time = current time + 3 days
                time_t expires_time = time(NULL) + 3 * 24 * 60 * 60;
                struct tm *expires_time_info = gmtime(&expires_time);
                char expires_time_str[100];
                strftime(expires_time_str, sizeof(expires_time_str), "%a, %d %b %Y %H:%M:%S %Z", expires_time_info);
                sprintf(success_response, "HTTP/1.1 200 OK\r\nContent-Length: %d\n\rCache-control: no-store\r\nContent-Type: %s\r\nContent-Language: en-us\r\nLast-Modified: %s\r\nExpires: %s\r\n\r\n", get_file_size(file_path), get_content_type(file_path), get_last_modification_date_time(file_path), expires_time_str);
                printf("%s\n", success_response);
                send_output(client_socket, success_response, strlen(success_response));
                sleep(1);
                // Send file contents
                memset(buffer, 0, sizeof(buffer));
                while ((bytes_received = read(fd, buffer, sizeof(buffer))) > 0)
                {
                    send_output(client_socket, buffer, bytes_received);
                    memset(buffer, 0, sizeof(buffer));
                }
                close(fd);
                char *ip = inet_ntoa(client_address.sin_addr);
                int portno = ntohs(client_address.sin_port);
                Access_log(ip, portno, "GET", file_path);
            }
            else if (strcmp(request_type, "PUT") == 0)
            {
                printf("%s\n", file_path);
                FILE *file = fopen(file_path, "w");
                if (file == NULL)
                {
                    perror("Error opening file\n");
                    if (errno == 2)
                        send_output(client_socket, NOT_FOUND_RESPONSE, strlen(NOT_FOUND_RESPONSE));
                    else if (errno == 13)
                        send_output(client_socket, FORBIDDEN_RESPONSE, strlen(FORBIDDEN_RESPONSE));
                    else
                        send_output(client_socket, BAD_REQUEST_RESPONSE, strlen(BAD_REQUEST_RESPONSE));
                    close(client_socket);
                    exit(1);
                }
                char *success_response = "HTTP/1.1 200 OK\r\nCache-control: no-store\r\nContent-Type: text/html\r\nContent-Language: en-us\r\n\r\n";
                printf("%s\n", success_response);
                send_output(client_socket, success_response, strlen(success_response));
                memset(buffer, 0, sizeof(buffer));
                int total_bytes_received = 0;
                sleep(1);
                while ((bytes_received = recv(client_socket, buffer, sizeof(buffer), 0)) > 0)
                {
                    total_bytes_received += bytes_received;
                    buffer[bytes_received] = '\0';
                    // This is a subsequent chunk of the response, so write it to the file
                    fwrite(buffer, sizeof(char), bytes_received, file);
                    memset(buffer, 0, sizeof(buffer));
                }
                fclose(file);
                char *ip = inet_ntoa(client_address.sin_addr);
                int portno = ntohs(client_address.sin_port);
                Access_log(ip, portno, "PUT", file_path);
            }
            else
            {
                printf("Error: Invalid request type");
                close(client_socket);
                send(client_socket, BAD_REQUEST_RESPONSE, strlen(BAD_REQUEST_RESPONSE), 0);
                exit(1);
            }
            free(tokens);
            free(req_type_dir);

            // Close file and client socket
            close(client_socket);
            exit(0);
        }
        else
        {
            close(client_socket);
        }
    }

    return 0;
}
