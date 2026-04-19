#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "comms.h"

/*
Get the first non-loopback IPv4 address of the host.  If no such address is found, return the loopback address
Warning: This function assumes the host has at least one non-loopback IPv4 address.
The first non-loopback IPv4 address is determined by iterating through the list of network interfaces and selecting the first one that is not in the 127.0.0.0/8 subnet.
 */
struct in_addr get_my_ip(void)
{
    struct ifaddrs *ifaddr, *ifa;

    if (getifaddrs(&ifaddr) == 0)
    {
        for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next)
        {
            if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
            {
                struct sockaddr_in *candidate_addr;
                unsigned long candidate_ip;

                candidate_addr = (struct sockaddr_in *)ifa->ifa_addr;
                candidate_ip = ntohl(candidate_addr->sin_addr.s_addr);

                /* Check non-loopback IPv4(not 127.0.0.0/8) */
                if ((candidate_ip & 0xFF000000UL) != 0x7F000000UL)
                {
                    freeifaddrs(ifaddr);
                    return candidate_addr->sin_addr;
                }
            }
        }
    }

    perror("get_my_ip failed");
    exit(EXIT_FAILURE);
}

void broadcast(void)
{
    int sock;
    char my_ip[INET_ADDRSTRLEN];
    struct in_addr my_addr;
    struct sockaddr_in broadcast_addr;

    my_addr = get_my_ip();
    inet_ntop(AF_INET, &my_addr, my_ip, sizeof(my_ip));

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);
    broadcast_addr.sin_port = htons(BROADCAST_PORT);

    /* Socket Option Broadcast */
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &SOCKOPT, sizeof(SOCKOPT)) < 0)
    {
        perror("broadcast sock setsockopt SO_BROADCAST failed");
        exit(EXIT_FAILURE);
    }

    /* Socket Option Reuse Address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &SOCKOPT, sizeof(SOCKOPT)) < 0)
    {
        perror("broadcast sock setsockopt SO_REUSEADDR failed");
        exit(EXIT_FAILURE);
    }

    /* Send the broadcast message (IP) */
    sendto(sock, my_ip, strlen(my_ip), 0, (const struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr));
    printf("Broadcast sent.\n");
    close(sock);
}

/* Return -1 if failed */
int listen_for_connection(void)
{
    int sock, accepted_sock;
    struct sockaddr_in listen_addr, peer_addr;
    struct timeval timeout;
    socklen_t len;

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    /* Socket Option Reuse Address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &SOCKOPT, sizeof(SOCKOPT)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        exit(EXIT_FAILURE);
    }

    /* Socket Option Timeout */
    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = LISTEN_TIMEOUT_SEC;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt SO_RCVTIMEO failed");
        exit(EXIT_FAILURE);
    }

    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(PORT);

    if (bind(sock, (const struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 1) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Awaiting connection request\n");
    len = sizeof(peer_addr);
    accepted_sock = accept(sock, (struct sockaddr *)&peer_addr, &len);

    /* The listener uses SO_RCVTIMEO only to bound discovery wait.
       Clear it on the accepted chat socket so chat reads do not timeout. */
    if (accepted_sock >= 0)
    {
        memset(&timeout, 0, sizeof(timeout));
        if (setsockopt(accepted_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0)
        {
            perror("setsockopt accepted_sock SO_RCVTIMEO reset failed");
            close(accepted_sock);
            accepted_sock = -1;
        }
    }

    close(sock);
    return accepted_sock;
}

int connect_to(const struct in_addr *target_ip)
{
    int sock;
    struct timeval timeout;
    struct sockaddr_in target_addr;

    memset(&target_addr, 0, sizeof(target_addr));
    target_addr.sin_family = AF_INET;
    target_addr.sin_addr = *target_ip;
    target_addr.sin_port = htons(PORT);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    /* Socket Option Reuse Address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &SOCKOPT, sizeof(SOCKOPT)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        exit(EXIT_FAILURE);
    }

    /* Socket Option Timeout */
    memset(&timeout, 0, sizeof(timeout));
    timeout.tv_sec = LISTEN_TIMEOUT_SEC;
    if (setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout)) < 0)
    {
        perror("setsockopt SO_SNDTIMEO failed");
        exit(EXIT_FAILURE);
    }

    if (connect(sock, (struct sockaddr *)&target_addr, sizeof(target_addr)) < 0)
    {
        perror("connect");
        return -1;
    }

    return sock;
}

/* Listen for broadcast UDP messages at port BROADCAST_PORT */
void listen_for_broadcast(struct sockaddr_in *source_addr, char *buffer, size_t buffer_size)
{
    int sock;
    struct sockaddr_in listen_addr;
    socklen_t len;
    int n;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    /* Socket Option Reuse Address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &SOCKOPT, sizeof(SOCKOPT)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        exit(EXIT_FAILURE);
    }

    /* Socket Option Broadcast */
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &SOCKOPT, sizeof(SOCKOPT)) < 0)
    {
        perror("setsockopt SO_BROADCAST failed");
        exit(EXIT_FAILURE);
    }

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = INADDR_ANY;
    listen_addr.sin_port = htons(BROADCAST_PORT);

    if (bind(sock, (const struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    len = sizeof(*source_addr);
    n = recvfrom(
        sock, (void *)buffer, buffer_size,
        MSG_WAITALL, (struct sockaddr *)source_addr, &len);

    buffer[n] = '\0';
    close(sock);
}