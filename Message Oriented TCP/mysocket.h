#ifndef _MYSOCKET_H
#define _MYSOCKET_H

#include <stdio.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netinet/in.h>
#include <arpa/inet.h>

#define SOCK_MyTCP SOCK_STREAM
#define T 5

struct Recv_Table
{
    // char message[10][5000];
    void *message[10];
    int length[10];
    int start, end, count;
};


struct Send_Table
{
    // char message[10][5000];
    void *message[10];
    int length[10];
    int start, end, count;
};


int my_socket(int domain, int type, int protocol);
int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_listen(int sockfd, int backlog);
int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen);
int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen);
int my_close(int sockfd);
int my_send(int sockfd, const void *buf, size_t len, int flags);
int my_recv(int sockfd, void *buf, size_t len, int flags);

#endif