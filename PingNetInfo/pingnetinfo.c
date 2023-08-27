/**
 * @file pingnetinfo.c
 * @brief Ping a network and get network information
 * @author Nikhil Saraswat (20CS10039) and Amit Kumar (20CS30003)
 * @date 2023-04-08
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <unistd.h>
#include <assert.h>
#include <poll.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>


#define ERROR(msg, ...) printf("\033[1;31m[ERROR] " msg " \033[0m\n", ##__VA_ARGS__);
#define SUCCESS(msg, ...) printf("\033[1;36m[SUCCESS] " msg " \033[0m\n", ##__VA_ARGS__);
#define INFO(msg, ...) printf("\033[1;34m[INFO] " msg " \033[0m\n", ##__VA_ARGS__);
#define DEBUG(msg, ...) printf("\033[1;32m[DEBUG] " msg "\033[0m", ##__VA_ARGS__);

uint16_t checksum(uint16_t *buffer, int size)
{
    // calculate checksum
    uint64_t cksum = 0; // 64 bit to avoid overflow
    while (size > 1)
    {                             // 16 bit at a time
        cksum += *buffer++;       // add 16 bit to cksum
        size -= sizeof(uint16_t); // decrease size by 16 bit
    }
    if (size)
    {                                // if size is odd
        cksum += *(uint8_t *)buffer; // add last 8 bit
    }
    cksum = (cksum >> 16) + (cksum & 0xffff); // add carry
    cksum += (cksum >> 16);                   // add carry
    return (uint16_t)(~cksum);                // return 1's complement
}

int get_local_ip(char *ip)
{ // get local IP address
    struct ifaddrs *ifaddr, *ifa;
    int family, s;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return -1;
    }

    /* Walk through linked list, maintaining head pointer so we can free list later */
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr == NULL)
            continue;

        family = ifa->ifa_addr->sa_family;

        /* For IPv4 addresses */
        if (family == AF_INET)
        {
            s = getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
            if (s != 0)
            {
                // printf("getnameinfo() failed: %s\n", gai_strerror(s));
                ERROR("getnameinfo() failed: %s", gai_strerror(s));
                return -1;
            }

            /* Check if the IP address is not 127.0.0.1 */
            if (strcmp(host, "127.0.0.1") != 0)
            {
                strcpy(ip, host);
                freeifaddrs(ifaddr);
                return 0;
            }
        }
    }

    freeifaddrs(ifaddr);
    return -1;
}

void print_ip_header(struct iphdr *ip_header)
{
    // print IP header
    printf("IP header info:\n");
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
    printf("| %40s | %40s | %40s |\n", "Version", "IHL", "Type of Service");
    printf("| %40d | %40d | %40d |\n", ip_header->version, ip_header->ihl, ip_header->tos);
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
    printf("|  %60s  |  %60s |\n", "Total Length", "Identification");
    printf("|  %60d  |  %60d |\n", ntohs(ip_header->tot_len), ntohs(ip_header->id));
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
    printf("| %40s | %40s | %40s |\n", "Flags", "Fragment Offset", "Time to Live");
    printf("| %40d | %40d | %40d |\n", (ntohs(ip_header->frag_off) & 0xE000) >> 13, ntohs(ip_header->frag_off) & 0x1FFF, ip_header->ttl);
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
    printf("|  %60s  |  %60s |\n", "Protocol", "Header Checksum");
    printf("|  %60d  |  %60d |\n", ip_header->protocol, ntohs(ip_header->check));
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
    printf("|       %120s |\n", "Source IP Address");
    // if source IP address is 0.0.0.0 then print local IP address
    if (ip_header->saddr == 0)
    {
        // get local IP address
        char local_ip[20];
        get_local_ip(local_ip);
        printf("|       %120s |\n", local_ip);
    }
    else
    {
        // print source IP address
        printf("|       %120s |\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
    }
    // printf("|       %120s |\n", inet_ntoa(*(struct in_addr *)&ip_header->saddr));
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
    printf("|       %120s |\n", "Destination IP Address");
    printf("|       %120s |\n", inet_ntoa(*(struct in_addr *)&ip_header->daddr));
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
}

void print_icmp_header(struct icmphdr *icmp_header)
{
    // print ICMP header
    printf("ICMP header info:\n");
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
    printf("| %40s | %40s | %40s |\n", "Type", "Code", "Checksum");
    printf("| %40d | %40d | %40d |\n", icmp_header->type, icmp_header->code, ntohs(icmp_header->checksum));
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
    printf("|  %60s  |  %60s |\n", "Identifier", "Sequence Number");
    printf("|  %60d  |  %60d |\n", ntohs(icmp_header->un.echo.id), ntohs(icmp_header->un.echo.sequence));
    printf("----------------------------------------------------------------------------------------------------------------------------------\n");
}

int check_host_name_or_ip(char *arg)
{
    // check whether arg is a x.y.z.w IP address or a hostname
    struct sockaddr_in sa;
    int result = inet_pton(AF_INET, arg, &(sa.sin_addr)); // convert IP address to binary form
    return result != 0;                                   // return 1 if it is a valid IP address, otherwise return 0
}

double abs_val(double x)
{
    // return absolute value of x
    if (x < 0)
    {
        return -x;
    }
    return x;
}

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        // fprintf(stderr, "Usage: %s <hostname or IP address> <number of probes(n)> <time difference between two probes(T)>\n", argv[0]);
        ERROR("Usage: %s <hostname or IP address> <number of probes(n)> <time difference between two probes(T)>\n", argv[0]);
        exit(EXIT_FAILURE);
    }
    int n = atoi(argv[2]);
    int T = atoi(argv[3]);
    struct sockaddr_in dest_addr;
    memset(&dest_addr, 0, sizeof(dest_addr));
    dest_addr.sin_family = AF_INET;
    dest_addr.sin_port = htons(2000);
    if (!check_host_name_or_ip(argv[1]))
    {
        // if it is not a valid IP address, then it must be a hostname
        // resolve the hostname to an IP address
        struct hostent *host = gethostbyname(argv[1]);
        if (host == NULL)
        {
            // perror("Error: could not resolve hostname");
            ERROR("Error: could not resolve hostname");
            exit(EXIT_FAILURE);
        }
        dest_addr.sin_addr = *(struct in_addr *)host->h_addr_list[0]; // this is the IP address of the destination
        SUCCESS("Resolved hostname %s to IP address %s\n", argv[1], inet_ntoa(dest_addr.sin_addr));
    }
    else
    {
        // if it is a valid IP address, then convert it to a binary address
        inet_pton(AF_INET, argv[1], &(dest_addr.sin_addr));
    }
    int sockfd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (sockfd < 0)
    {
        // perror("Error: could not create socket");
        ERROR("Error: could not create socket");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in src_addr;
    memset(&src_addr, 0, sizeof(src_addr));
    src_addr.sin_family = AF_INET;
    src_addr.sin_addr.s_addr = INADDR_ANY;

    int ON = 1;
    if (setsockopt(sockfd, IPPROTO_IP, IP_HDRINCL, &ON, sizeof(ON)) < 0)
    {
        // perror("Error: could not set IP_HDRINCL");
        ERROR("Error: could not set IP_HDRINCL");
        exit(EXIT_FAILURE);
    }
    char buffer[1024];
    memset(buffer, 0, sizeof(buffer));
    struct iphdr *ip_header = (struct iphdr *)buffer;
    struct icmphdr *icmp_header = (struct icmphdr *)(buffer + sizeof(struct iphdr));

    // add data to the icmp header
    char data[1024];
    memset(data, 0, sizeof(data));
    strcpy(data, "I am Nikhil Saraswat\n");
    memcpy((buffer + sizeof(struct iphdr) + sizeof(struct icmphdr)), data, sizeof(data));

    ip_header->version = 4;                                                            // IPv4
    ip_header->ihl = 5;                                                                // 5 * 32 bits = 20 bytes
    ip_header->tos = 0;                                                                // type of service
    ip_header->tot_len = sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(data); // total length
    ip_header->id = 0;                                                                 // identification
    ip_header->frag_off = 0;                                                           // fragment offset
    ip_header->ttl = 64;                                                               // time to live
    ip_header->protocol = IPPROTO_ICMP;                                                // protocol
    ip_header->check = 0;                                                              // checksum
    ip_header->saddr = src_addr.sin_addr.s_addr;                                       // source address
    ip_header->daddr = dest_addr.sin_addr.s_addr;                                      // destination address

    ip_header->check = checksum((unsigned short *)ip_header, sizeof(struct iphdr)); // checksum

    print_ip_header(ip_header);

    icmp_header->type = ICMP_ECHO; // echo request
    icmp_header->code = 0;         // code
    icmp_header->un.echo.id = 0;   // id

    int seq = 0;                         // sequence number
    icmp_header->un.echo.sequence = seq; // sequence number

    icmp_header->checksum = 0;
    icmp_header->checksum = checksum((unsigned short *)icmp_header, sizeof(struct icmphdr) + sizeof(data)); // checksum

    print_icmp_header(icmp_header);

    assert(checksum((unsigned short *)icmp_header, sizeof(struct icmphdr) + sizeof(data)) == 0);
    assert(checksum((unsigned short *)ip_header, sizeof(struct iphdr)) == 0);

    double RTT[64];                                    // latency of each hop
    double RTT_with_data[64];                          // bandwidth of each hop
    struct sockaddr_in hop_addr[64];                   // address of each hop
    char local_ip[64];                                 // local ip address
    get_local_ip(local_ip);                            // get local ip address
    hop_addr[0].sin_addr.s_addr = inet_addr(local_ip); // first hop is local ip address
    // trace route to the destination
    int ttl = 1;
    while (1)
    {
        if(ttl > 64)
        {
            ERROR("Error: could not trace route to destination\n");
            exit(EXIT_FAILURE);
        }
        // update ttl in ip header
        ip_header->ttl = ttl;
        int id = ttl;
        // send 5 packets to finalise the next hop
        struct sockaddr_in next_hop_addr[5];
        for (int i = 0; i < 5; i++)
        {
            // send the packet
            if (sendto(sockfd, buffer, sizeof(buffer), 0, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
            {
                // printf("Error: could not send packet of size 0\n");
                // perror("Error: could not send packet");
                ERROR("Error: could not send packet\n");
                exit(EXIT_FAILURE);
            }

            // receive the packet
            struct sockaddr_in recv_addr;
            socklen_t recv_addr_len = sizeof(recv_addr);
            char recv_buffer[1024];
            struct pollfd fds;
            fds.fd = sockfd;
            fds.events = POLLIN;
            int timeout = 5000; // 5 seconds
            int ret = poll(&fds, 1, timeout);
            if (ret == -1)
            {
                // perror("Error: poll");
                ERROR("Error: poll");
                exit(EXIT_FAILURE);
            }
            if (ret == 0)
            {
                printf("* * * *\n");
                i--;
                continue;
            }
            int bytes_received = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
            if (bytes_received < 0)
            {
                // perror("Error: could not receive packet");
                ERROR("Error: could not receive packet");
                exit(EXIT_FAILURE);
            }
            // save the next hop address
            next_hop_addr[i] = recv_addr;
            sleep(1);
        }
        // check if the next hop is the same for all 5 packets
        int same = 1;
        for (int i = 1; i < 5; i++)
        {
            if (next_hop_addr[i].sin_addr.s_addr != next_hop_addr[i - 1].sin_addr.s_addr)
            {
                same = 0;
                break;
            }
        }
        if (same)
        {
            // store the next hop address
            hop_addr[id] = next_hop_addr[0];
            RTT[id] = 0;
            RTT_with_data[id] = 0;
            printf("%d. %s\n", ttl, inet_ntoa(hop_addr[ttl].sin_addr));
            // now send 5 packets without data to find latency of link
            double total_latency = 0;
            double total_data_sending_time = 0;
            // printf("Sending %d pings to %s to check latency\n", n, inet_ntoa(next_hop_addr[0].sin_addr));
            INFO("Sending %d probes to %s to check latency\n", n, inet_ntoa(next_hop_addr[0].sin_addr));
            seq = 0;
            int cnt = 0;
            struct timeval start[3 * n + 1], end[3 * n + 1];
            memset(start, 0, sizeof(start));
            memset(end, 0, sizeof(end));
            for (int i = 0; i < n; i++)
            {
                // send dataless packet
                ip_header->id = id;
                ip_header->tot_len = sizeof(struct iphdr) + sizeof(struct icmphdr);
                // ip_header->daddr = next_hop_addr[0].sin_addr.s_addr;
                ip_header->check = 0;
                ip_header->check = checksum((unsigned short *)ip_header, sizeof(struct iphdr)); // checksum
                assert(checksum((unsigned short *)ip_header, sizeof(struct iphdr)) == 0);
                icmp_header->un.echo.id = id;
                icmp_header->un.echo.sequence = ++seq;
                icmp_header->checksum = 0;
                icmp_header->checksum = checksum((unsigned short *)icmp_header, sizeof(struct icmphdr)); // checksum
                assert(checksum((unsigned short *)icmp_header, sizeof(struct icmphdr)) == 0);
                if (!i)
                {
                    printf("IP header and ICMP header of first packet:\n");
                    print_ip_header(ip_header);
                    print_icmp_header(icmp_header);
                }
                // send the packet
                if (sendto(sockfd, buffer, ip_header->tot_len, 0, (struct sockaddr *)&next_hop_addr[0], sizeof(next_hop_addr[0])) < 0)
                {
                    // printf("Error: could not send packet of size 0\n");
                    // perror("Error: could not send packet");
                    ERROR("Error: could not send packet\n");
                    exit(EXIT_FAILURE);
                }
                // mark the time
                // struct timeval start_time;
                gettimeofday(&start[seq], NULL);
                // receive the packet
                struct sockaddr_in recv_addr;
                socklen_t recv_addr_len = sizeof(recv_addr);
                char recv_buffer[1024];
                struct pollfd fds;
                fds.fd = sockfd;
                fds.events = POLLIN;
                int timeout = 5000; // 5 seconds
                int flag = 1;
                while (1)
                {
                    struct timeval poll_time_start;
                    gettimeofday(&poll_time_start, NULL);
                    int ret = poll(&fds, 1, timeout);
                    if (ret == -1)
                    {
                        // perror("Error: poll");
                        ERROR("Error: poll");
                        exit(EXIT_FAILURE);
                    }
                    if (ret == 0)
                    {
                        break;
                    }
                    struct timeval poll_time_end;
                    gettimeofday(&poll_time_end, NULL);
                    int bytes_received = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
                    if (bytes_received < 0)
                    {
                        // perror("Error: could not receive packet");
                        ERROR("Error: could not receive packet");
                        exit(EXIT_FAILURE);
                    }
                    // struct timeval end_time;
                    cnt++;
                    flag = 0;
                    struct iphdr *recv_ip_header = (struct iphdr *)recv_buffer;
                    struct icmphdr *recv_icmp_header = (struct icmphdr *)(recv_buffer + sizeof(struct iphdr));

                    if (recv_icmp_header->type == 11)
                    {
                        // timeout 
                        struct iphdr *recv_ip_header = (struct iphdr *)(recv_buffer + sizeof(struct iphdr) + sizeof(struct icmphdr));
                        int seq_num;
                        if (recv_ip_header->protocol == IPPROTO_ICMP)
                        {
                            struct icmphdr *recv_icmp_header = (struct icmphdr *)(recv_buffer + sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr));
                            seq_num = recv_icmp_header->un.echo.sequence;
                            gettimeofday(&end[seq_num], NULL);
                            INFO("<seq = %d and id = %d>\n", seq_num, recv_icmp_header->un.echo.id);
                        }
                        else if (recv_ip_header->protocol == IPPROTO_TCP)
                        {
                            printf("TCP packet received\n");
                            continue;
                        }
                        else if (recv_ip_header->protocol == IPPROTO_UDP)
                        {
                            printf("UDP packet received\n");
                            continue;
                        }
                        else
                        {
                            continue;
                        }
                        double latency = (end[seq_num].tv_sec - start[seq_num].tv_sec) * 1000000L + (end[seq_num].tv_usec - start[seq_num].tv_usec);
                        // printf("latency = %f micro-sec\n", latency);
                        RTT[seq_num] += latency;
                    }
                    else if (recv_icmp_header->type == 0)
                    {
                        // echo reply
                        int recv_seq = recv_icmp_header->un.echo.sequence;
                        INFO("<seq = %d and id = %d>\n", recv_seq, recv_icmp_header->un.echo.id);

                        gettimeofday(&end[recv_seq], NULL);
                        double latency = (end[recv_seq].tv_sec - start[recv_seq].tv_sec) * 1000000L + (end[recv_seq].tv_usec - start[recv_seq].tv_usec);
                        // printf("latency = %f micro-sec\n", latency);
                        RTT[recv_seq] += latency;
                    }
                    INFO("Received packet from %s\n", inet_ntoa(recv_addr.sin_addr));
                    print_ip_header(recv_ip_header);
                    print_icmp_header(recv_icmp_header);
                    timeout -= (poll_time_end.tv_sec - poll_time_start.tv_sec) * 1000000L + (poll_time_end.tv_usec - poll_time_start.tv_usec);
                    if (timeout <= 1e-6)
                    {
                        break;
                    }
                }
                if (flag)
                {
                    printf("* * * *\n");
                    i--;
                    continue;
                }
                sleep(T);
            }
            int cnt_with_data = 0;
            // printf("Sending %d pings to %s to check bandwidth\n", n, inet_ntoa(next_hop_addr[0].sin_addr));
            INFO("Sending %d probes to %s to check bandwidth\n", n, inet_ntoa(next_hop_addr[0].sin_addr));
            for (int i = 0; i < n; i++)
            {
                // send data packet
                ip_header->id = id;
                ip_header->tot_len = sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(data);
                // ip_header->daddr = next_hop_addr[0].sin_addr.s_addr;
                ip_header->check = 0;
                ip_header->check = checksum((unsigned short *)ip_header, sizeof(struct iphdr)); // checksum
                assert(checksum((unsigned short *)ip_header, sizeof(struct iphdr)) == 0);
                icmp_header->un.echo.id = id;
                icmp_header->un.echo.sequence = ++seq;
                icmp_header->checksum = 0;
                icmp_header->checksum = checksum((unsigned short *)icmp_header, sizeof(struct icmphdr) + sizeof(data));
                assert(checksum((unsigned short *)icmp_header, sizeof(struct icmphdr) + sizeof(data)) == 0);

                if (!i)
                {
                    printf("IP header and ICMP header of first packet:\n");
                    print_ip_header(ip_header);
                    print_icmp_header(icmp_header);
                }
                // send the packet
                if (sendto(sockfd, buffer, ip_header->tot_len, 0, (struct sockaddr *)&hop_addr[ttl], sizeof(hop_addr[ttl])) < 0)
                {
                    // printf("Error: could not send packet of size 0\n");
                    // perror("Error: could not send packet");
                    ERROR("Error: could not send packet");
                    exit(EXIT_FAILURE);
                }
                // mark the time
                // struct timeval start_time;
                gettimeofday(&start[seq], NULL);
                // receive the packet
                struct sockaddr_in recv_addr;
                socklen_t recv_addr_len = sizeof(recv_addr);
                char recv_buffer[1024];
                struct pollfd fds;
                fds.fd = sockfd;
                fds.events = POLLIN;
                int timeout = 5000; // 5 seconds
                int flag = 1;
                while (1)
                {
                    struct timeval poll_time_start;
                    gettimeofday(&poll_time_start, NULL);
                    int ret = poll(&fds, 1, timeout);
                    if (ret == -1)
                    {
                        // perror("Error: poll");
                        ERROR("Error: poll");
                        exit(EXIT_FAILURE);
                    }
                    if (ret == 0)
                    {
                        // printf("Timeout\n");
                        break;
                    }
                    struct timeval poll_time_end;
                    gettimeofday(&poll_time_end, NULL);
                    int bytes_received = recvfrom(sockfd, recv_buffer, sizeof(recv_buffer), 0, (struct sockaddr *)&recv_addr, &recv_addr_len);
                    if (bytes_received < 0)
                    {
                        // perror("Error: could not receive packet");
                        ERROR("Error: could not receive packet");
                        exit(EXIT_FAILURE);
                    }
                    // struct timeval end_time;
                    cnt_with_data++;
                    flag = 0;
                    struct iphdr *recv_ip_header = (struct iphdr *)recv_buffer;
                    struct icmphdr *recv_icmp_header = (struct icmphdr *)(recv_buffer + sizeof(struct iphdr));

                    if (recv_icmp_header->type == 11)
                    {
                        struct iphdr *recv_ip_header = (struct iphdr *)(recv_buffer + sizeof(struct iphdr) + sizeof(struct icmphdr));
                        int seq_num;
                        if (recv_ip_header->protocol == IPPROTO_ICMP)
                        {
                            struct icmphdr *recv_icmp_header = (struct icmphdr *)(recv_buffer + sizeof(struct iphdr) + sizeof(struct icmphdr) + sizeof(struct iphdr));
                            seq_num = recv_icmp_header->un.echo.sequence;
                            gettimeofday(&end[seq_num], NULL);
                            INFO("<seq = %d and id = %d>\n", seq_num, recv_icmp_header->un.echo.id);
                        }
                        else if (recv_ip_header->protocol == IPPROTO_TCP)
                        {
                            printf("TCP packet received\n");
                            continue;
                        }
                        else if (recv_ip_header->protocol == IPPROTO_UDP)
                        {
                            printf("UDP packet received\n");
                            continue;
                        }
                        else
                        {
                            continue;
                        }
                        double latency = (end[seq_num].tv_sec - start[seq_num].tv_sec) * 1000000L + (end[seq_num].tv_usec - start[seq_num].tv_usec);
                        // printf("latency = %f micro-sec\n", latency);
                        RTT_with_data[seq_num] += latency;
                    }
                    else if (recv_icmp_header->type == 0)
                    {
                        int recv_seq = recv_icmp_header->un.echo.sequence;
                        INFO("<seq = %d and id = %d>\n", recv_seq, recv_icmp_header->un.echo.id);
                        gettimeofday(&end[recv_seq], NULL);
                        double latency = (end[recv_seq].tv_sec - start[recv_seq].tv_sec) * 1000000L + (end[recv_seq].tv_usec - start[recv_seq].tv_usec);
                        // printf("latency = %f micro-sec\n", latency);
                        RTT_with_data[recv_seq] += latency;
                    }

                    INFO("Received packet from %s\n", inet_ntoa(recv_addr.sin_addr));
                    print_ip_header(recv_ip_header);
                    print_icmp_header(recv_icmp_header);
                }
                if (flag)
                {
                    printf("* * * *\n");
                    i--;
                    continue;
                }
                sleep(T);
            }

            char x_ip[100], y_ip[100];
            strcpy(x_ip, inet_ntoa(hop_addr[ttl - 1].sin_addr));
            strcpy(y_ip, inet_ntoa(hop_addr[ttl].sin_addr));
            // printf("Size of data: %ld\n", sizeof(data));

            double latency = abs_val(RTT[id] - RTT[id - 1]) / cnt;
            double latency_with_data = abs_val(RTT_with_data[id] - RTT_with_data[id - 1]) / cnt_with_data;
            double bandwidth = (sizeof(data)) / abs_val(latency_with_data - latency);
            // printf("Latency: %f micro-sec and Bandwidth: %f MBps of link %s -> %s\n", latency, bandwidth, x_ip, y_ip);
            SUCCESS("Latency: %f micro-sec and Bandwidth: %f MBps of link %s -> %s", latency, bandwidth, x_ip, y_ip);
        }
        else
        {
            printf("* * * *\n");
            continue;
        }
        ttl++;
        if (hop_addr[ttl - 1].sin_addr.s_addr == dest_addr.sin_addr.s_addr)
        {
            // printf("Destination reached\n");
            SUCCESS("Destination reached");
            break;
        }
    }
    close(sockfd);

    return 0;
}