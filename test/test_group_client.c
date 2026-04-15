#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "user_manager.h"
#include "directory_manager.h"
#include "group.h"
#include "group_comms.h"
#include "group_user_comms.h"
#include "lmp.h"

/*
Running Command:
gcc test_group_client.c group_user_comms.c group.c uid.c directory_manager.c -o test_group_client && ./test_group_client
 */
int main()
{
    struct sockaddr_in server_addr;
    GroupDiscoveryReplyMsg reply;
    char nickname[NICKNAME_MAX_LEN];
    char default_nickname[] = "test_user";

    /* Ask for program_username */
    printf("Enter your nickname (Press enter for default): ");
    fgets(nickname, sizeof(nickname), stdin);
    nickname[strcspn(nickname, "\n")] = '\0'; /* Remove newline character */

    /* If user enters an empty nickname, set nickname as "test_user" */
    if (strlen(nickname) == 0)
    {
        strncpy(nickname, default_nickname, sizeof(nickname) - 1);
        nickname[sizeof(nickname) - 1] = '\0'; /* Ensure null termination */
    }

    /* Init user */
    set_program_username(nickname);
    init_user_information();

    if (user_UDP_to_group_server(&server_addr, &reply) == 1)
    {
        int tcp_socket;
        GroupMember member = {0};
        printf("Server replied from sin_family: %d\n", server_addr.sin_family);
        printf("\n--- Server Discovered! ---\n");
        printf("Group ID: %s\n", reply.info.group_UID);
        printf("Group Name: %s\n", reply.info.group_name);

        /* Prepare GroupMember struct */
        strncpy(member.uid, user.uid, sizeof(member.uid) - 1);
        member.uid[sizeof(member.uid) - 1] = '\0'; /* Ensure null termination */
        strncpy(member.nickname, nickname, sizeof(member.nickname) - 1);
        member.nickname[sizeof(member.nickname) - 1] = '\0'; /* Ensure null termination */

        if ((tcp_socket = user_TCP_to_group_server(&server_addr)) > 0)
        {
            /* Receive from server */
            /*
            char buffer[1024];
            int bytes_received;
            */
            printf("\n--- Connected to Server via TCP ---\n");

            /* Send User's information, GroupMember */
            if (send(tcp_socket, &member, sizeof(member), 0) == -1)
            {
                perror("send failed");
                close(tcp_socket);
                return 1;
            }

            chat(tcp_socket, "user");

            printf("\n--- Closing TCP Connection ---\n");
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