#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "uid.h"
#include "group.h"
#include "group_user.h"
#include "group_comms.h"
#include "group_user_comms.h"
#include "lmp.h"
#include "commands_registry.h"
#include "directory_manager.h"
#include "group_commands.h"

Group user_group;
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

    recv_len = recvfrom(sock, reply, sizeof(*reply), 0, (struct sockaddr *)server_addr, &addr_len);
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

/* Middleware function to handle chat functionality */
/* int sock: socket file descriptor */
/* const char *role: role of the user (e.g., "server", "client") */
void group_chat(int sock, const char *role)
{
    char peer_ip[INET_ADDRSTRLEN];

    (void)role;

    if (get_peer_ip(sock, peer_ip, sizeof(peer_ip)) == 0)
        printf("Connected from IP: %s\n", peer_ip);
    else
        strncpy(peer_ip, "unknown_peer", sizeof(peer_ip) - 1);

    peer_ip[sizeof(peer_ip) - 1] = '\0';

    init_commands();     /* Initialize all commands */
    user_group_MODE = 1; /* Set mode to user_group */
    chat_loop_user(sock, peer_ip, "");
}

int group_lmp_recv(int fd, uint8_t *type_out, char *buf, uint32_t bufsize, uint32_t *len_out, int *sender_out)
{
    lmp_header_t hdr;
    uint32_t plen;

    int header_bytes = read_all(fd, &hdr, sizeof(hdr));
    if (header_bytes < 0)
        return -1;
    if (header_bytes == 0)
        return -2; /* temp code for EOF (connection closed)*/

    /* Checking valid header */
    if (hdr.magic[0] != LMP_MAGIC_0 || hdr.magic[1] != LMP_MAGIC_1 || header_bytes < sizeof hdr)
        return -3;

    plen = ntohl(hdr.payload_len);
    if (plen >= bufsize)
        return -4;

    if (plen > 0 && read_all(fd, buf, plen) <= 0)
        return -5;
    buf[plen] = '\0';

    *type_out = hdr.type;
    *len_out = plen;
    *sender_out = hdr.reserved;
    return 0;
}

static void *receiver(void *arg)
{
    LMPContext *ctx = (LMPContext *)arg;
    uint8_t type;
    char buf[4096];
    uint32_t len;
    int sender_i;

    int status_code;

    while (1)
    {
        status_code = group_lmp_recv(ctx->sock, &type, buf, sizeof(buf), &len, &sender_i);

        /* Amend ctx for dispatch_recv function */
        memcpy(ctx->peer_nick, &user_group.members[sender_i].nickname, sizeof(char) * NICKNAME_MAX_LEN);
        memcpy(ctx->peer_uid, &user_group.members[sender_i].uid, sizeof(char) * (UID_LENGTH + 1));

        if (status_code != 0)
            break;
        printf("\r\033[K");
        if (dispatch_recv(type, buf, len, ctx) == COMMAND_UNRECOGNIZED)
            printf("[warning]: unrecognized message type 0x%02X\n", type);
        print_prompt(ctx);
    }

    fprintf(stderr, "lmp_recv: status %d\n", status_code);
    printf("*** Peer disconnected.\n");
    return NULL;
}

void chat_loop_user(int sock, const char *server_ip, const char *history_path)
{
    pthread_t recv_thread;
    char line[1024];
    LMPContext ctx;
    /* FILE *fp; */
    ctx.sock = sock;

    strncpy(ctx.my_nick, "You", sizeof(ctx.my_nick) - 1);
    ctx.my_nick[sizeof(ctx.my_nick) - 1] = '\0';
    /* strncpy(ctx.peer_nick, "Peer", sizeof(ctx.peer_nick) - 1); */
    /* ctx.peer_nick[sizeof(ctx.peer_nick) - 1] = '\0'; */

    /* Load saved nickname if it exists */
    /*
    fp = open_file_in_user_directory("nick.txt", "r");
    if (fp != NULL)
    {
        fscanf(fp, "%63s", ctx.my_nick);
        fclose(fp);
    }
    */

    /*
    strncpy(ctx.peer_ip, server_ip ? server_ip : "unknown_peer", sizeof(ctx.peer_ip) - 1);
    ctx.peer_ip[sizeof(ctx.peer_ip) - 1] = '\0';
    */
    /*
    strncpy(ctx.history_path, history_path ? history_path : "", sizeof(ctx.history_path) - 1);
    ctx.history_path[sizeof(ctx.history_path) - 1] = '\0';
    */
    /* ctx.peer_uid[0] = '\0'; */
    /* ctx.history_loaded = 0; */

    strncpy(ctx.my_uid, get_uid(), sizeof(ctx.my_uid) - 1);
    ctx.my_uid[sizeof(ctx.my_uid) - 1] = '\0';
    ctx.peer_uid_ready = 0;

    /* Initialize user_member_info with server */
    user_grp_init_send(sock);

    pthread_create(&recv_thread, NULL, receiver, &ctx);

    /* send GroupMember information here */

    while (1)
    {
        print_prompt(&ctx);
        if (!fgets(line, sizeof(line), stdin))
            break;
        strip_newline(line);

        if (line[0] != '/')
        {
            size_t len = strlen(line);
            if (len + 5 < sizeof(line))
            {
                memmove(line + 5, line, len + 1); /* include '\0' */
                memcpy(line, "/msg ", 5);
            }
            else
            {
                printf("Message too long\n");
                continue;
            }
        }
        switch (dispatch_send(line + 1, &ctx))
        {
        case COMMAND_SUCCESS:
            break;
        case COMMAND_ERROR:
            printf("Error executing '%s'\n", line);
            break;
        case COMMAND_UNRECOGNIZED:
            printf("Unrecognized command '%s'\n", line);
            break;
        }
    }

    shutdown(sock, SHUT_WR);
    pthread_join(recv_thread, NULL);
}
