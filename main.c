/* Main C file for Socket Speak */
#include <stdlib.h>
#include <stdio.h>
#include "comms.h"
#include "user_manager.h"
#include "lmp.h"
#include "main.h"

void flag_handler(int argc, char *argv[])
{
    /* TODO: Update flag_handler to handle -u flag for username */
    if (argc != 1)
    {
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[])
{
    int accepted_sock, n;
    UserInformation *user_info;

    flag_handler(argc, argv);
    broadcast();

    if ((accepted_sock = listen_for_connection()) == -1)
    {
        int connecting_sock;
        char buffer[MAXLINE];
        struct sockaddr_in source_addr;
        char sender_ip[INET_ADDRSTRLEN];

        printf("Timed out - no data received within 5 seconds.\n");
        printf("Becoming server\n");

        /* Blocking function: Listen for broadcast messages */
        listen_for_broadcast(&source_addr, buffer, sizeof(buffer));

        inet_ntop(AF_INET, &(source_addr.sin_addr), sender_ip, INET_ADDRSTRLEN);
        printf("Received UDP packet from %s! Message: %s\n", sender_ip, buffer);

        if ((connecting_sock = connect_to(&source_addr.sin_addr)) == -1)
        {
            perror("connect_to failed");
            exit(EXIT_FAILURE);
        }
        printf("Connection established with %s.\n", sender_ip);

        /* Init user information */
        set_program_username("server");
        user_info = init_user_information();
        printf("User UID: %s\n", user_info->uid);

        chat(connecting_sock, "server");
        close(connecting_sock);
    }
    else
    {
        /* Init user information */
        set_program_username("client");
        user_info = init_user_information();
        printf("User UID: %s\n", user_info->uid);

        printf("Connection accepted.\n");
        chat(accepted_sock, "client");
        close(accepted_sock);
    }
    return 0;
}