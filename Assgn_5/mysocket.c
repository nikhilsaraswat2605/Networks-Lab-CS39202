#include "mysocket.h"

// threads for sending and receiving messages
pthread_t R, S;
// mutex for Received_Message and Sent_Message
pthread_mutex_t Recv_mutex, Send_mutex;
struct Send_Table Sent_Message;
struct Recv_Table Received_Message;

int newsockfd; 

int min(int a, int b)
{ // returns the minimum of two integers
    if (a < b)
        return a;
    return b;
}

// This function is used to send messages to the socket
void *Send_Message(void *sockfd)
{

    while (1)
    {
        // wait for T seconds and then if Sent_Message is not empty, send it using send()
        sleep(T);
        if (newsockfd == -1e9)
        {
            pthread_testcancel(); // cancel the thread if the socket is closed
            continue;
        }
        if (Sent_Message.count == 0)
        {
            pthread_testcancel(); // cancel the thread if Sent_Message is empty
            continue;
        }
        // lock the mutex
        pthread_mutex_lock(&Send_mutex);
        while (Sent_Message.count > 0)
        {
            // add length of message to the message as the first 4 bytes of the message
            int len = Sent_Message.length[Sent_Message.start];
            char *message = (char *)malloc(sizeof(char) * 5005); // 5000 bytes for message and 4 bytes for length
            memset(message, 0, 5005);                         // initialize the message to null
            int temp = len;
            int i = 3;
            while (i >= 0)
            { // convert the length to string
                message[i] = temp % 10 + '0';
                temp /= 10;
                i--;
            }
            message[i] = '\0'; // add null character at the end
            for (int i = 0; i < len; i++)
            { // add the message to the message
                message[i + 4] = ((char *)Sent_Message.message[Sent_Message.start])[i]; // add the message to the message
            }
            // send the message in loop until all the bytes are sent
            int n = 0;
            while (n < len + 4)
            { // send the message in loop until all the bytes are sent
                int m = send(newsockfd, message + n, len + 4 - n, 0);
                if (m == 0)
                { // if the connection is closed, exit the thread
                    printf("Connection closed by client %d in Send_Message\n", newsockfd);
                    pthread_exit(NULL);
                }
                else if (m < 0)
                { // if the message cannot be sent, exit the thread
                    printf("Cannot send message to socket %d in Send_Message\n", newsockfd);
                    pthread_exit(NULL);
                }
                n += m; // increment the number of bytes sent
            }
            // unlock the mutex
            Sent_Message.start = (Sent_Message.start + 1) % 10;
            Sent_Message.count--; // decrement the count of Sent_Message
            free(message);
        }
        pthread_mutex_unlock(&Send_mutex); // unlock the mutex
    }
}

// This function is used to receive messages from the socket
void *Receive_Message(void *sockfd)
{
    while (1)
    { // loop until the socket is closed
        if (newsockfd == -1e9)
        {
            pthread_testcancel(); // cancel the thread if the socket is closed
            continue;
        }
        if (Received_Message.count == 10)
        {
            pthread_testcancel(); // cancel the thread if Received_Message is full
            continue;
        }
        // wait on revc() call and if message is received, add it to Received_Message
        char *message = (char *)malloc(sizeof(char) * 5005);
        memset(message, 0, 5005);
        // loop until atleast 4 bytes are received
        int n = 0;
        while (n < 4)
        {
            int m = recv(newsockfd, message + n, 4 - n, 0); // receive the message
            if (m == 0)
            {
                printf("Connection closed by client %d in Receive_Message\n", newsockfd); // if the connection is closed, exit the thread
                pthread_exit(NULL);
            }
            else if (m < 0)
            {
                printf("Cannot receive message from socket %d in Receive_Message\n", newsockfd); // if the message cannot be received, exit the thread
                pthread_exit(NULL); 
            }
            n += m; // increment the number of bytes received
        }
        message[n] = '\0';
        // get the length of the message from the first 4 bytes
        int len = 0;
        for (int i = 0; i < 4; i++)
        { // get the length of the message from the first 4 bytes
            len = len * 10 + (message[i] - '0');
        }
        // loop until all the bytes are received
        Received_Message.length[Received_Message.end] = len;
        while (n < len + 4)
        {
            int m = recv(newsockfd, message + n, len + 4 - n, 0); // receive the message
            if (m == 0)
            {
                printf("Connection closed by client %d in Receive_Message\n", newsockfd); // if the connection is closed, exit the thread
                pthread_exit(NULL);
            }
            else if (m < 0)
            {
                printf("Cannot receive message from socket %d in Receive_Message\n", newsockfd); // if the message cannot be received, exit the thread
                pthread_exit(NULL);
            }
            n += m; // increment the number of bytes received
        }
        // add the message to Received_Message
        // lock the mutex
        pthread_mutex_lock(&Recv_mutex); // lock the mutex
        for (int i = 4; i < len + 4; i++)
            ((char *)Received_Message.message[Received_Message.end])[i - 4] = message[i]; // add the message to Received_Message
        Received_Message.end = (Received_Message.end + 1) % 10;
        Received_Message.count++;
        // unlock the mutex
        pthread_mutex_unlock(&Recv_mutex); // unlock the mutex
        free(message); // free the memory
    }
}

int my_socket(int domain, int type, int protocol)
{
    // create socket
    if (type != SOCK_MyTCP)
    { // only SOCK_MyTCP is supported
        printf("Error: Only SOCK_MyTCP is supported\n");
        exit(1);
    }
    int sockfd = socket(domain, type, protocol); // create socket
    if (sockfd < 0)
    {
        return sockfd; // return the socket descriptor
    }
    newsockfd = -1e9; // initialize newsockfd
    return sockfd; // return the socket descriptor
}

int my_bind(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    return bind(sockfd, addr, addrlen); // bind the socket
}

int my_listen(int sockfd, int backlog)
{
    return listen(sockfd, backlog); // listen on the socket
}

int my_connect(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    newsockfd = -1e9; // initialize newsockfd
    // allocate memory for Received_Message and Sent_Message
    Received_Message.start = Received_Message.end = Received_Message.count = 0; // initialize Received_Message
    for (int i = 0; i < 10; i++)
    {
        Received_Message.message[i] = (char *)malloc(sizeof(char) * 5005); // allocate memory for Received_Message
    }
    Sent_Message.start = Sent_Message.end = Sent_Message.count = 0;
    for (int i = 0; i < 10; i++)
    {
        Sent_Message.message[i] = (char *)malloc(sizeof(char) * 5005); // allocate memory for Sent_Message
    }
    // intialize mutex
    pthread_mutex_init(&Recv_mutex, NULL);
    pthread_mutex_init(&Send_mutex, NULL);
    // create thread to receive message
    pthread_create(&R, NULL, Receive_Message, &sockfd);
    // create thread to send message
    pthread_create(&S, NULL, Send_Message, &sockfd);
    newsockfd = sockfd; // set newsockfd
    return connect(sockfd, addr, addrlen);
}

int my_accept(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    newsockfd = -1e9; // initialize newsockfd
    // allocate memory for Received_Message and Sent_Message
    Received_Message.start = Received_Message.end = Received_Message.count = 0; // initialize Received_Message
    for (int i = 0; i < 10; i++)
    {
        Received_Message.message[i] = (char *)malloc(sizeof(char) * 5005); // allocate memory for Received_Message
    }
    Sent_Message.start = Sent_Message.end = Sent_Message.count = 0;
    for (int i = 0; i < 10; i++)
    {
        Sent_Message.message[i] = (char *)malloc(sizeof(char) * 5005); // allocate memory for Sent_Message
    }
    // intialize mutex
    pthread_mutex_init(&Recv_mutex, NULL); // initialize mutex
    pthread_mutex_init(&Send_mutex, NULL); // initialize mutex
    // create thread to receive message
    pthread_create(&R, NULL, Receive_Message, &sockfd); // create thread to receive message
    // create thread to send message
    pthread_create(&S, NULL, Send_Message, &sockfd); // create thread to send message
    newsockfd = accept(sockfd, addr, addrlen); // accept the connection
    return newsockfd;
}

int my_recv(int sockfd, void *buf, size_t len, int flags)
{
    while (Received_Message.count == 0) // wait until there is a message
        ;
    // copy the message to buf
    // lock the mutex
    pthread_mutex_lock(&Recv_mutex); // lock the mutex
    for (int i = 0; i < min(len, Received_Message.length[Received_Message.start]); i++)
        ((char *)buf)[i] = ((char *)Received_Message.message[Received_Message.start])[i]; // copy the message to buf
    int msg_len = Received_Message.length[Received_Message.start];
    Received_Message.start = (Received_Message.start + 1) % 10; // update the start of Received_Message
    Received_Message.count--; // update the count of Received_Message
    // unlock the mutex
    pthread_mutex_unlock(&Recv_mutex); // unlock the mutex
    return min(len, msg_len); // return the number of bytes received
}

int my_send(int sockfd, const void *buf, size_t len, int flags)
{
    // get blocked until Sent_Message is not full
    while (Sent_Message.count == 10)
        ;
    // copy the message to Sent_Message
    // lock the mutex
    pthread_mutex_lock(&Send_mutex);
    Sent_Message.length[Sent_Message.end] = len;
    for (int i = 0; i < len; i++)
    {
        ((char *)Sent_Message.message[Sent_Message.end])[i] = ((char *)buf)[i]; // copy the message to Sent_Message
    }
    Sent_Message.end = (Sent_Message.end + 1) % 10; // update the end of Sent_Message
    Sent_Message.count++;
    // unlock the mutex
    pthread_mutex_unlock(&Send_mutex);
    return len;
}

int my_close(int sockfd)
{
    // deallocate memory for Received_Message and Sent_Message
    sleep(T+1);
    pthread_cancel(R); // cancel the thread
    pthread_cancel(S); // cancel the thread
    sleep(1); // wait for the thread to be cancelled
    if (sockfd == newsockfd)
    {
        for (int i = 0; i < 10; i++)
        {
            free(Received_Message.message[i]);
            free(Sent_Message.message[i]);
        }
    }
    newsockfd = -1e9; // reset newsockfd
    // destroy mutex
    pthread_mutex_destroy(&Recv_mutex);
    pthread_mutex_destroy(&Send_mutex);

    // join threads
    pthread_join(R, NULL);
    pthread_join(S, NULL);

    return close(sockfd);
}
