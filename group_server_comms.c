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

/*
For Server, create group_server_UDP_reply as a seperate thread
*/
pthread_t server_create_UDP_reply_thread(GroupInfo *group_info)
{
    /* Create a thread that runs group_server_UDP_reply with the provided group_info */
    /* This thread will handle incoming UDP discovery requests and reply with the group information */
    pthread_t udp_thread;
    pthread_create(&udp_thread, NULL, (void *(*)(void *))group_server_UDP_reply, group_info);
    return udp_thread;
}

/* For Server */
/* Parameters: GroupInfo */
void group_server_UDP_reply(GroupInfo *group_info)
{
    /* Persistent UDP listener at GROUP_UDP_PORT for discovery */
    /* When a client sends the MAGIC_WORD, reply with a fixed-size binary discovery message */
    int sock;
    struct sockaddr_in listen_addr, client_addr;
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

    printf("Server listening on port %d for discovery requests...\n", GROUP_UDP_PORT);

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));

        recv_len = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            (struct sockaddr *)&client_addr, &addr_len);

        if (recv_len > 0)
        {
            buffer[recv_len] = '\0';

            if (strcmp(buffer, MAGIC_WORD) == 0)
            {
                GroupDiscoveryReplyMsg reply;
                /* TODO: Write to Log? */
                printf("Received discovery request from %s\n",
                       inet_ntoa(client_addr.sin_addr));

                /* Create GroupDiscoveryReplyMsg */
                memset(&reply, 0, sizeof(reply));
                reply.magic[0] = (GROUP_DISCOVERY_MAGIC >> 8) & 0xFF;
                reply.magic[1] = GROUP_DISCOVERY_MAGIC & 0xFF;
                reply.status = 0; /* ready */
                reply.info = *group_info;

                if (sendto(sock, &reply, sizeof(reply), 0,
                           (struct sockaddr *)&client_addr, addr_len) < 0)
                {
                    perror("Sendto failed");
                }
                else
                {
                    printf("Sent discovery response to %s\n",
                           inet_ntoa(client_addr.sin_addr));
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