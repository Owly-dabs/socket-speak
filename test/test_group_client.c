#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include "group.h"
#include "group_comms.h"
#include "group_user_comms.h"

/*
Running Command:
gcc test_group_client.c group_user_comms.c group.c uid.c directory_manager.c -o test_group_client && ./test_group_client
 */
int main()
{
    struct sockaddr_in server_addr;
    GroupDiscoveryReplyMsg reply;
    if (user_UDP_to_group_server(&server_addr, &reply) == 1)
    {
        int tcp_socket;
        printf("Server replied from sin_family: %d\n", server_addr.sin_family);
        printf("\n--- Server Discovered! ---\n");
        printf("Group ID: %s\n", reply.info.group_UID);
        printf("Group Name: %s\n", reply.info.group_name);

        if ((tcp_socket = user_TCP_to_group_server(&server_addr)) > 0)
        {
            printf("TCP connection to server established successfully.\n");
            close(tcp_socket);
        }
        else
        {
            printf("Failed to establish TCP connection to server.\n");
        }
    }
    else
    {
        printf("\nFailed to discover server within the timeout period.\n");
    }
    return 0;
}