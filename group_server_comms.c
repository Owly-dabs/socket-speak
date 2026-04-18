#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <poll.h>
#include "group.h"
#include "group_comms.h"
#include "group_commands.h"
#include "group_server.h"
#include "group_server_comms.h"
#include "lmp.h"
#include "commands_registry.h"

Group current_group;

/*
For Server, create group_server_UDP_reply as a seperate thread
*/
static void *group_server_UDP_reply_entry(void *arg)
{
    GroupInfo local_group_info;
    local_group_info = *((GroupInfo *)arg);
    free(arg);
    group_server_UDP_reply(&local_group_info);
    return NULL;
}

pthread_t server_create_UDP_reply_thread()
{
    /* Create a thread that runs group_server_UDP_reply with the provided group_info */
    /* This thread will handle incoming UDP discovery requests and reply with the group information */
    pthread_t udp_thread;
    GroupInfo *thread_group_info;
    int create_status;
    GroupInfo *group_info = &current_group.info;

    thread_group_info = (GroupInfo *)malloc(sizeof(GroupInfo));
    if (thread_group_info == NULL)
    {
        perror("malloc failed");
        return (pthread_t)0;
    }

    *thread_group_info = *group_info;
    create_status = pthread_create(&udp_thread, NULL, group_server_UDP_reply_entry, (void *)thread_group_info);
    if (create_status != 0)
    {
        fprintf(stderr, "pthread_create failed: %s\n", strerror(create_status));
        free(thread_group_info);
        return (pthread_t)0;
    }

    return udp_thread;
}

/* For Server */
/* Parameters: GroupInfo */
void group_server_UDP_reply(GroupInfo *group_info)
{
    /* Persistent UDP listener at GROUP_UDP_PORT for discovery */
    /* When a user sends the MAGIC_WORD, reply with a fixed-size binary discovery message */
    int sock;
    struct sockaddr_in listen_addr, user_addr;
    socklen_t addr_len = sizeof(struct sockaddr_in);
    char buffer[1024];
    int recv_len;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    listen_addr.sin_port = htons(GROUP_UDP_PORT);

    if (bind(sock, (struct sockaddr *)&listen_addr, sizeof(listen_addr)) < 0)
    {
        perror("Bind failed");
        close(sock);
        exit(EXIT_FAILURE);
    }

    printf("[Group UDP] Server listening on port %d for discovery requests...\n", GROUP_UDP_PORT);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        recv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            (struct sockaddr *)&user_addr, &addr_len);

        if (recv_len > 0)
        {
            buffer[recv_len] = '\0';

            if (strcmp(buffer, MAGIC_WORD) == 0)
            {
                GroupDiscoveryReplyMsg reply;
                /* TODO: Write to Log? */
                printf("[Group UDP] Received discovery request from %s\n",
                       inet_ntoa(user_addr.sin_addr));

                /* Create GroupDiscoveryReplyMsg */
                memset(&reply, 0, sizeof(reply));
                reply.magic[0] = (GROUP_DISCOVERY_MAGIC >> 8) & 0xFF;
                reply.magic[1] = GROUP_DISCOVERY_MAGIC & 0xFF;
                reply.status = 0; /* ready */
                reply.info = *group_info;

                if (sendto(sock, &reply, sizeof(reply), 0,
                           (struct sockaddr *)&user_addr, addr_len) < 0)
                {
                    perror("Sendto failed");
                }
                else
                {
                    printf("[Group UDP] Sent discovery response to %s\n",
                           inet_ntoa(user_addr.sin_addr));
                }
            }
        }
        else if (recv_len < 0)
        {
            perror("Recvfrom failed");
            break;
        }
    }

    close(sock);
}

/*
 * Add a new file descriptor to the set.
 */
void add_to_pfds(struct pollfd **pfds, int newfd, int *fd_count,
                 int *fd_size)
{
    /* If we don't have room, add more space in the pfds array */
    if (*fd_count == *fd_size)
    {
        *fd_size *= 2; /* Double it */
        *pfds = realloc(*pfds, sizeof(**pfds) * (*fd_size));
    }

    (*pfds)[*fd_count].fd = newfd;
    (*pfds)[*fd_count].events = POLLIN; /* Check ready-to-read */
    (*pfds)[*fd_count].revents = 0;

    (*fd_count)++;
}

/*
 * Remove a file descriptor at a given index from the set.
 */
void del_from_pfds(struct pollfd pfds[], int i, int *fd_count)
{
    /* Copy the one from the end over this one */
    pfds[i] = pfds[*fd_count - 1];

    (*fd_count)--;
}

int send_new_group_to_users(int listener, int sender_fd,
                            int *fd_count, struct pollfd **pfds)
{
    int i;
    LMPContext ctx;
    for (i = 0; i < *fd_count; i++)
    {
        int dest_fd = (*pfds)[i].fd;
        ctx.sock = dest_fd;
        /* Except the listener */
        if (dest_fd != listener)
        {
            if ((grp_obj_send(LMP_GRP_OBJ, "", &ctx)) < 0)
            {
                perror("grp_obj_send");
                return -1;
            }
        }
    }

    return 0;
}
/*
 * Handle incoming connections.
 */
void handle_new_connection(int listener, int *fd_count, int *fd_size, struct pollfd **pfds)
{
    struct sockaddr_in remoteaddr; /* Client address */
    socklen_t addrlen = sizeof(remoteaddr);
    int newfd = accept(listener, (struct sockaddr *)&remoteaddr, &addrlen);

    if (newfd == -1)
    {
        perror("accept");
        return;
    }

    add_to_pfds(pfds, newfd, fd_count, fd_size);
    printf("[Group TCP] Accepted connection from %s\n", inet_ntoa(remoteaddr.sin_addr));
    printf("[Group TCP] Awaiting group member information...\n");
}

int group_lmp_send(int fd, uint8_t type, const char *payload, uint32_t len, int sender)
{
    lmp_header_t hdr;
    hdr.magic[0] = LMP_MAGIC_0;
    hdr.magic[1] = LMP_MAGIC_1;
    hdr.type = type;
    hdr.reserved = (uint8_t)sender;
    hdr.payload_len = htonl(len);

    if (send(fd, &hdr, sizeof(hdr), 0) < 0)
        return -1;
    if (len > 0 && send(fd, payload, len, 0) < 0)
        return -1;
    return 0;
}

/*
 * Handle regular client data or client hangups.
 */
void handle_client_data(int listener, int *fd_count, struct pollfd *pfds, int *pfd_i)
{
    uint8_t type;
    char buf[4096];
    uint32_t len;

    int sender_fd = pfds[*pfd_i].fd;
    int recv_status = lmp_recv(pfds[*pfd_i].fd, &type, buf, sizeof buf, &len);

    int j;
    int sender_connection_index;
    GroupMember new_member;

    if (recv_status < 0)
    { /* Got error or connection closed by client */
        if (recv_status == -2)
        {
            /* Connection closed */
            printf("[Server] socket %d hung up\n", sender_fd);
        }
        else
        {
            fprintf(stderr, "recv_status = %d\n", recv_status);
            perror("recv");
        }

        close(pfds[*pfd_i].fd); /* Bye! */

        del_from_pfds(pfds, *pfd_i, fd_count);

        /* reexamine the slot we just deleted */
        (*pfd_i)--;
    }
    else
    {
        printf("[Server] recv from fd %d: %.*s\n", sender_fd, len, buf);
        sender_connection_index = *pfd_i - 1; /* pollfd have one extra listener */
        switch (type)
        {
        case LMP_NICK: /* Check if type == /nick */
            printf("[Server] Update username from %s to %s\n", current_group.members[sender_connection_index].nickname, buf);
            strncpy(current_group.members[sender_connection_index].nickname, buf, NICKNAME_MAX_LEN - 1);
            current_group.members[sender_connection_index].nickname[NICKNAME_MAX_LEN - 1] = '\0'; /* Ensure null termination */

            if (send_new_group_to_users(listener, sender_fd, fd_count, &pfds) != 0)
            {
                perror("send_new_group_to_users");
            }
            break;
        case LMP_GRP_INIT_MEMBER: /* When user initializes their member information */
            new_member = *(GroupMember *)buf;
            printf("[Server] Received new member information: UID=%s, Nickname=%s\n", new_member.uid, new_member.nickname);
            current_group.members[current_group.member_count] = new_member;
            current_group.member_count++;

            if (send_new_group_to_users(listener, sender_fd, fd_count, &pfds) != 0)
            {
                perror("send_new_group_to_users");
            }
            break;
        case LMP_MSG: /* Save message to history.txt */
            if (save_message_to_history(current_group.members[sender_connection_index].uid, buf) != 0)
            {
                perror("Failed to save message to history");
            }
        default:
            for (j = 0; j < *fd_count; j++)
            {
                int dest_fd = pfds[j].fd;
                /* Except the listener and ourselves */
                if (dest_fd != listener && dest_fd != sender_fd)
                {
                    if (group_lmp_send(dest_fd, type, buf, len, sender_connection_index) == -1)
                    {
                        perror("send");
                    }
                }
            }
            break;
        }
    }
}

/*
 * Process all existing connections.
 */
void process_connections(int listener, int *fd_count, int *fd_size, struct pollfd **pfds)
{
    int i;
    for (i = 0; i < *fd_count; i++)
    {

        /* Check if someone's ready to read */
        if ((*pfds)[i].revents & (POLLIN | POLLHUP))
        {
            /* We got one!! */

            if ((*pfds)[i].fd == listener)
            {
                /* If we're the listener, it's a new connection */
                handle_new_connection(listener, fd_count, fd_size,
                                      pfds);
            }
            else
            {
                /* Otherwise we're just a regular client */
                handle_client_data(listener, fd_count, *pfds, &i);
            }
        }
    }
}

void close_all_socks(struct pollfd *pfds, int *fd_count)
{
    while ((*fd_count) > 0)
    {
        close(pfds[*fd_count - 1].fd);
        del_from_pfds(pfds, *fd_count - 1, fd_count);
    }
}

int get_listener_socket()
{
    int sock, reuse = 1;
    struct sockaddr_in server_addr;

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(GROUP_TCP_PORT);

    /* Create Socket */
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("socket failed");
        return -1;
    }

    /* Socket Option Reuse Address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        return -1;
    }

    /* Bind to port */
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        return -1;
    }

    if (listen(sock, 5) < 0) /* at most 5 requests should come in at one time... */
    {
        perror("listen failed");
        return -1;
    }

    return sock;
}

int group_server_TCP_listen()
{
    int sock;

    int fd_size = MAX_GROUP_CONNECTIONS;
    int fd_count = 0;
    struct pollfd *pfds = malloc(sizeof *pfds * fd_size);

    /* Create Listener Socket */
    sock = get_listener_socket();
    if (sock < 0)
    {
        fprintf(stderr, "error getting listening socket\n");
        exit(EXIT_FAILURE);
    }

    /* Add the listener to set; */
    /* Report ready to read on incoming connection */
    pfds[0].fd = sock;
    pfds[0].events = POLLIN;

    fd_count = 1; /* For the listener */

    printf("[Group TCP] Awaiting connection request\n");
    for (;;)
    {
        int poll_count = poll(pfds, fd_count, -1);

        if (poll_count == -1)
        {
            perror("poll");
            exit(1);
        }

        /* Run through connections looking for data to read */
        process_connections(sock, &fd_count, &fd_size, &pfds);
    }

    close_all_socks(pfds, &fd_count); /* temp function until handle_client_data is up */
    close(sock);
    free(pfds);
    return 0;
}
