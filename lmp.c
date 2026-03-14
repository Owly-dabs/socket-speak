#include "lmp.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>

static int read_all(int fd, void *buf, size_t n)
{
    size_t total = 0;
    char *p = (char *)buf;
    ssize_t r;
    while (total < n)
    {
        r = read(fd, p + total, n - total);
        if (r <= 0)
            return -1;
        total += (size_t)r;
    }
    return 0;
}

int lmp_send(int fd, uint8_t type, const char *payload, uint32_t len)
{
    lmp_header_t hdr;
    hdr.magic[0] = LMP_MAGIC_0;
    hdr.magic[1] = LMP_MAGIC_1;
    hdr.type = type;
    hdr.reserved = 0x00;
    hdr.payload_len = htonl(len);

    if (send(fd, &hdr, sizeof(hdr), 0) < 0)
        return -1;
    if (len > 0 && send(fd, payload, len, 0) < 0)
        return -1;
    return 0;
}

int lmp_recv(int fd, uint8_t *type_out, char *buf, uint32_t bufsize, uint32_t *len_out)
{
    lmp_header_t hdr;
    uint32_t plen;

    if (read_all(fd, &hdr, sizeof(hdr)) < 0)
        return -1;
    if (hdr.magic[0] != LMP_MAGIC_0 || hdr.magic[1] != LMP_MAGIC_1)
        return -1;

    plen = ntohl(hdr.payload_len);
    if (plen >= bufsize)
        return -1;

    if (plen > 0 && read_all(fd, buf, plen) < 0)
        return -1;
    buf[plen] = '\0';

    *type_out = hdr.type;
    *len_out = plen;
    return 0;
}

static void strip_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

static void *receiver(void *arg)
{
    int sock = *(int *)arg;
    uint8_t type;
    char buf[4096];
    uint32_t len;
    char peer_nick[64];

    strncpy(peer_nick, "peer", sizeof(peer_nick) - 1);
    peer_nick[sizeof(peer_nick) - 1] = '\0';

    while (lmp_recv(sock, &type, buf, sizeof(buf), &len) == 0)
    {
        if (type == LMP_MSG)
            printf("[%s]: %s\n", peer_nick, buf);
        else if (type == LMP_NICKNAME)
        {
            strncpy(peer_nick, buf, sizeof(peer_nick) - 1);
            peer_nick[sizeof(peer_nick) - 1] = '\0';
            printf("*** Peer is now known as: %s\n", peer_nick);
            lmp_send(sock, LMP_ACK, "nickname ack", 12);
        }
        else if (type == LMP_ACK)
            printf("[ack]: %s\n", buf);
        else if (type == LMP_ERROR)
            printf("[error]: %s\n", buf);
    }
    printf("*** Peer disconnected.\n");
    return NULL;
}

void chat_loop(int sock)
{
    pthread_t recv_thread;
    char line[1024];
    char nickname[64];
    char client_ip_str[INET_ADDRSTRLEN]; /* IPv4 address string */

    if (get_peer_ip(sock, client_ip_str, sizeof(client_ip_str)) == 0)
    {
        printf("Connected from IP: %s\n", client_ip_str);
    }

    strncpy(nickname, "me", sizeof(nickname) - 1);
    nickname[sizeof(nickname) - 1] = '\0';

    pthread_create(&recv_thread, NULL, receiver, &sock);

    while (fgets(line, sizeof(line), stdin))
    {
        strip_newline(line);
        if (strncmp(line, "/nick ", 6) == 0)
        {
            strncpy(nickname, line + 6, sizeof(nickname) - 1);
            nickname[sizeof(nickname) - 1] = '\0';
            lmp_send(sock, LMP_NICKNAME, nickname, (uint32_t)strlen(nickname));
        }
        else
        {
            lmp_send(sock, LMP_MSG, line, (uint32_t)strlen(line));
        }
    }
    shutdown(sock, SHUT_WR);
    pthread_join(recv_thread, NULL);
}

int get_peer_ip(int sockfd, char *ip_str, size_t ip_str_len)
{
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);

    if (getpeername(sockfd, (struct sockaddr *)&peer_addr, &peer_addr_len) == -1)
    {
        perror("getpeername failed");
        return -1;
    }

    /* Check address family and convert to string */
    if (peer_addr.ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)&peer_addr;
        if (inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, ip_str_len) == NULL)
        {
            perror("inet_ntop (IPv4) failed");
            return -1;
        }
    }
    else if (peer_addr.ss_family == AF_INET6)
    {
        printf("Connecting with IPv6 address, not supported in this server\n");
        return -1;
    }
    else
    {
        printf("Unknown address family\n");
        return -1;
    }

    return 0;
}