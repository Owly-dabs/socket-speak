/* Main C file for Socket Speak */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "comms.h"
#include "user_manager.h"
#include "directory_manager.h"
#include "lmp.h"

void flag_handler(int argc, char *argv[])
{
    if (argc == 1)
    {
        printf("No username provided (-u). Using default username: 'default'\n");
        return;
    }

    if (argc == 3 && strcmp(argv[1], "-u") == 0 && argv[2][0] != '\0')
    {
        set_program_username(argv[2]);
        return;
    }

    fprintf(stderr, "Usage: %s [-u username]\n", argv[0]);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[])
{
    int accepted_sock;

    flag_handler(argc, argv);
    init_user_information();
    broadcast();

    if ((accepted_sock = listen_for_connection()) == -1)
    {
        int connecting_sock;
        char buffer[MAXLINE];
        struct sockaddr_in source_addr;

        printf("Timed out - no data received within 5 seconds.\n");
        printf("Becoming server\n");

        /* Blocking function: Listen for broadcast messages */
        /* Future usage for handling received broadcast message `buffer` */
        listen_for_broadcast(&source_addr, buffer, sizeof(buffer));

        if ((connecting_sock = connect_to(&source_addr.sin_addr)) == -1)
        {
            perror("connect_to failed");
            exit(EXIT_FAILURE);
        }

        chat(connecting_sock, "server");
        close(connecting_sock);
    }
    else
    {
        printf("Connection accepted.\n");
        chat(accepted_sock, "client");
        close(accepted_sock);
    }
    return 0;
}