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
#include "group_server_comms.h"
#include "lmp.h"
#include "commands_registry.h"

GroupConnection group_connections[MAX_GROUP_CONNECTIONS];
Group current_group;
int connection_count = 0;

static void append_text(char *dest, size_t dest_size, const char *src)
{
    size_t dest_len;
    size_t src_len;
    size_t copy_len;

    if (dest_size == 0)
    {
        return;
    }

    dest_len = strlen(dest);
    if (dest_len >= dest_size - 1)
    {
        return;
    }

    src_len = strlen(src);
    copy_len = dest_size - dest_len - 1;
    if (src_len < copy_len)
    {
        copy_len = src_len;
    }

    memcpy(dest + dest_len, src, copy_len);
    dest[dest_len + copy_len] = '\0';
}

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

/* Add member to global connections */
void add_to_global_connections(GroupMember member, int user_sock)
{
    group_connections[connection_count].tcp_socket = user_sock;
    group_connections[connection_count].member = member;
    connection_count++;
    current_group.members[current_group.member_count] = member;
    current_group.member_count++;
    return;
}

/*
 * Send welcome message to newest connection
 * Returns success (0) or fail (1)
 */
int send_welcome_message(GroupMember member, int user_sock)
{
    int i;
    char response[1024];
    const char *nickname;
    const char *group_name;
    LMPContext ctx;
    ctx.sock = user_sock;
    nickname = (member.nickname[0] != '\0') ? member.nickname : "Unknown";
    group_name = (current_group.info.group_name[0] != '\0') ? current_group.info.group_name : "Unknown Group";

    response[0] = '\0';
    append_text(response, sizeof(response), "Hello ");
    append_text(response, sizeof(response), nickname);
    append_text(response, sizeof(response), ",\n");
    append_text(response, sizeof(response), "Welcome to group ");
    append_text(response, sizeof(response), group_name);
    append_text(response, sizeof(response), "\n");
    append_text(response, sizeof(response), "Current members:\n");

    for (i = 0; i < connection_count; i++)
    {
        const char *list_nickname;
        const char *list_uid;

        list_nickname = (group_connections[i].member.nickname[0] != '\0') ? group_connections[i].member.nickname : "Unknown";
        list_uid = (group_connections[i].member.uid[0] != '\0') ? group_connections[i].member.uid : "Unknown";

        append_text(response, sizeof(response), "- ");
        append_text(response, sizeof(response), list_nickname);
        append_text(response, sizeof(response), " (UID: ");
        append_text(response, sizeof(response), list_uid);
        append_text(response, sizeof(response), ")\n");
    }

    if ((lmp_send(user_sock, LMP_MSG, response, (uint32_t)strlen(response))) < 0)
        return -1;

    printf("Current number of members: %d\n", current_group.member_count);
    if ((grp_obj_send(LMP_GRP_OBJ, "", &ctx)) < 0)
        return -1;
    return 0;
}

/*
 * Handle incoming connections.
 */
void handle_new_connection(int listener, int *fd_count,
                           int *fd_size, struct pollfd **pfds)
{
    struct sockaddr_in remoteaddr; /* Client address */
    socklen_t addrlen;
    int newfd;                /* Newly accept()ed socket descriptor */
    GroupMember member = {0}; /* Initialize member with zeros */

    addrlen = sizeof remoteaddr;
    newfd = accept(listener, (struct sockaddr *)&remoteaddr,
                   &addrlen);

    if (newfd == -1)
    {
        perror("accept");
        return;
    }

    /* Continue if accept is successful */

    if (connection_count >= MAX_GROUP_CONNECTIONS)
    {
        fprintf(stderr, "Maximum connection limit reached. Cannot add more connections.\n");
        close(newfd);
        return;
    }

    /* Continue if both accept is successful and
        connection limit has not been reached */

    add_to_pfds(pfds, newfd, fd_count, fd_size);

    /* Receive member info (UID and nickname) */
    recv(newfd, (void *)&member, sizeof(member), 0);

    add_to_global_connections(member, newfd);

    printf("[Group TCP] Accepted connection from %s\n",
           inet_ntoa(remoteaddr.sin_addr));

    if (send_welcome_message(member, newfd) != 0)
    {
        perror("send");
        close(newfd);
        return;
    }
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
void handle_client_data(int listener, int *fd_count,
                        struct pollfd *pfds, int *pfd_i)
{
    int j;
    uint8_t type;
    char buf[4096];
    uint32_t len;

    int sender_connection_index;
    int sender_fd = pfds[*pfd_i].fd;
    int recv_status = lmp_recv(pfds[*pfd_i].fd, &type, buf, sizeof buf, &len);

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
    { /* We got some good data from a client */
        printf("[Server] recv from fd %d: %.*s\n", sender_fd,
               len, buf);
        /* Send to everyone! */
        sender_connection_index = sender_fd - 1;
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
    }
}

/*
 * Process all existing connections.
 */
void process_connections(int listener, int *fd_count, int *fd_size,
                         struct pollfd **pfds)
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
