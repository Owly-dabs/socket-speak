#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "uid.h"
#include "group.h"
#include "group_comms.h"
#include "group_user_comms.h"

/* For User */
/* Parameters: Pointer to store the server's address and reply message */
/* Returns: 1 if server found, 0 otherwise */
int user_UDP_to_group_server(struct sockaddr_in *server_addr, GroupDiscoveryReplyMsg *reply)
{
    int sock, recv_len, broadcast_enable = 1;
    struct sockaddr_in broadcast_addr, my_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    struct timeval tv;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        return 0;
    }

    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0)
    {
        perror("Error setting broadcast option");
        close(sock);
        return 0;
    }

    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    my_addr.sin_port = htons(0); /* Binding to 0 tells the OS to assign an ephemeral port */

    if (bind(sock, (struct sockaddr *)&my_addr, sizeof(my_addr)) < 0)
    {
        perror("Bind failed");
        close(sock);
        return 0;
    }

    memset(&tv, 0, sizeof(tv));
    tv.tv_sec = CLIENT_UDP_GROUP_TIMEOUT;
    if (setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0)
    {
        perror("Error setting timeout option");
        close(sock);
        return 0;
    }

    memset(&broadcast_addr, 0, sizeof(broadcast_addr));
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(GROUP_UDP_PORT);
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST); /* 255.255.255.255 */

    printf("Broadcasting to port %d...\n", GROUP_UDP_PORT);
    if (sendto(sock, MAGIC_WORD, strlen(MAGIC_WORD), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0)
    {
        perror("Sendto failed");
        close(sock);
        return 0;
    }

    printf("Waiting for server reply...\n");
    memset(server_addr, 0, sizeof(*server_addr));
    memset(reply, 0, sizeof(*reply));

    recv_len = recvfrom(sock, reply, sizeof(*reply), 0, (struct sockaddr *)&server_addr, &addr_len);
    close(sock);

    return recv_len == sizeof(*reply) ? 1 : 0;
}

/* For User */
/* Parameters: Pointer to the server's address */
/* Return socket, -1 on failure */
int user_TCP_to_group_server(struct sockaddr_in *server_addr)
{
    int tcp_sock, reuse = 1;
    /* Update server_addr port to the TCP port */
    server_addr->sin_family = AF_INET;
    server_addr->sin_port = htons(GROUP_TCP_PORT);

    /* Create TCP socket */
    tcp_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_sock < 0)
    {
        perror("TCP socket creation failed");
        return -1;
    }

    if (setsockopt(tcp_sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("Error setting reuseaddr option");
        close(tcp_sock);
        return -1;
    }

    /* Connect to the server */
    if (connect(tcp_sock, (struct sockaddr *)server_addr, sizeof(*server_addr)) < 0)
    {
        perror("TCP connection failed");
        close(tcp_sock);
        return -1;
    }

    return tcp_sock;
}