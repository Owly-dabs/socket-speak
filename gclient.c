#include <arpa/inet.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "user_manager.h"
#include "directory_manager.h"
#include "group.h"
#include "group_comms.h"
#include "group_user_comms.h"
#include "group_user.h"
#include "lmp.h"

extern GroupMember user_member_info; /* Global variable to hold the user's member information */

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
        printf("Server replied from sin_family: %d\n", server_addr.sin_family);
        printf("\n--- Server Discovered! ---\n");
        printf("Group ID: %s\n", reply.info.group_UID);
        printf("Group Name: %s\n", reply.info.group_name);

        /* Prepare GroupMember user_member_info */
        strncpy(user_member_info.uid, user.uid, sizeof(user_member_info.uid) - 1);
        user_member_info.uid[sizeof(user_member_info.uid) - 1] = '\0'; /* Ensure null termination */
        strncpy(user_member_info.nickname, nickname, sizeof(user_member_info.nickname) - 1);
        user_member_info.nickname[sizeof(user_member_info.nickname) - 1] = '\0'; /* Ensure null termination */

        if ((tcp_socket = user_TCP_to_group_server(&server_addr)) > 0)
        {
            printf("\n--- Connected to Server via TCP ---\n");
            group_chat(tcp_socket, "user");
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