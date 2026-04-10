#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "group.h"
#include "group_comms.h"
#include "group_server_comms.h"

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

void *handle_user_connection(void *arg)
{
    char response[1024];
    int user_sock = *((int *)arg);
    GroupMember member = {0}; /* Initialize member with zeros */
    int i;
    const char *nickname;
    const char *group_name;

    free(arg);

    /* Add user_sock to group_connections */
    group_connections[connection_count - 1].tcp_socket = user_sock;
    /* Note: The UID will be set later when the user sends their UID */

    /* Initial receive for UID and nickname if have */
    recv(user_sock, (void *)&member, sizeof(member) - 1, 0);

    /* Add user to group connections list */
    for (i = 0; i < connection_count; i++)
    {
        if (group_connections[i].tcp_socket == user_sock)
        {
            group_connections[i].member = member;
            break;
        }
    }

    /* Print to terminal */
    printf("[Group TCP] User connected: %s (UID: %s)\n", member.nickname, member.uid);

    /* Prepare welcome message */
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

    send(user_sock, response, strlen(response), 0);

    /* TODO: Handle when user disconnects */
    sleep(1000); /* Placeholder to keep the connection open for testing */
    close(user_sock);
    return NULL;
}

int group_server_TCP_listen()
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
        exit(EXIT_FAILURE);
    }

    /* Socket Option Reuse Address */
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        exit(EXIT_FAILURE);
    }

    /* Bind to port */
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(sock, 5) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    printf("[Group TCP] Awaiting connection request\n");
    /* For each accepted connection, spawn a new thread to handle the user */
    while (1)
    {
        struct sockaddr_in user_addr;
        socklen_t addr_len = sizeof(user_addr);
        int user_sock = accept(sock, (struct sockaddr *)&user_addr, &addr_len);
        pthread_t user_thread;
        int *user_sock_ptr;
        int create_status;
        if (user_sock < 0)
        {
            perror("accept failed");
            continue;
        }

        /* Create user thread */
        if (connection_count < MAX_GROUP_CONNECTIONS)
        {
            user_sock_ptr = (int *)malloc(sizeof(int));
            if (user_sock_ptr == NULL)
            {
                perror("malloc failed");
                close(user_sock);
                continue;
            }

            *user_sock_ptr = user_sock;
            create_status = pthread_create(&user_thread, NULL, handle_user_connection, (void *)user_sock_ptr);
            if (create_status != 0)
            {
                fprintf(stderr, "pthread_create failed: %s\n", strerror(create_status));
                free(user_sock_ptr);
                close(user_sock);
                continue;
            }

            pthread_detach(user_thread);
            connection_count++;
        }
        else
        {
            fprintf(stderr, "Maximum connection limit reached. Cannot add more connections.\n");
            close(user_sock);
        }

        printf("[Group TCP] Accepted connection from %s\n", inet_ntoa(user_addr.sin_addr));
    }
    close(sock);
    return 0;
}
