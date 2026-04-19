#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "group.h"
#include "group_server_comms.h"
#include "group_server.h"

extern Group current_group;

static void *tcp_listen_entry(void *arg)
{
    (void)arg;
    group_server_TCP_listen();
    return NULL;
}

/*
Running Command:
gcc test_group_server.c group_server_comms.c group.c uid.c directory_manager.c -o test_group_server && ./test_group_server
 */
int main(void)
{
    char *group_UID = "12345678";    /* Example UID (8 hex characters) */
    char *group_name = "Test Group"; /* Example group name */
    pthread_t udp_thread;
    pthread_t tcp_thread;
    char command[128];
    int create_status;

    init_group_server(group_UID);

    strcpy(current_group.info.group_UID, group_UID);
    current_group.info.group_UID[UID_LENGTH] = '\0'; /* Ensure null termination */
    strcpy(current_group.info.group_name, group_name);
    current_group.info.group_name[GROUP_NAME_MAX_LEN - 1] = '\0'; /* Ensure null termination */
    udp_thread = server_create_UDP_reply_thread();

    create_status = pthread_create(&tcp_thread, NULL, tcp_listen_entry, NULL);
    if (create_status != 0)
    {
        fprintf(stderr, "pthread_create failed: %s\n", strerror(create_status));
        pthread_cancel(udp_thread);
        pthread_join(udp_thread, NULL);
        return 1;
    }

    printf("Commands: /list, /close\n");

    while (fgets(command, sizeof(command), stdin) != NULL)
    {
        command[strcspn(command, "\r\n")] = '\0';

        if (strcmp(command, "/list") == 0)
        {
            /* TODO: Implement list command */
            printf("Not implemented yet.\n");
        }
        else if (strcmp(command, "/close") == 0)
        {
            /* TODO: Implement close command to close all connections in poll array */
            pthread_cancel(tcp_thread);
            pthread_cancel(udp_thread);
            pthread_join(tcp_thread, NULL);
            pthread_join(udp_thread, NULL);

            printf("Server closed.\n");
            return 0;
        }
        else if (command[0] != '\0')
        {
            printf("Unknown command: %s\n", command);
            printf("Commands: /list, /close\n");
        }
    }

    pthread_cancel(tcp_thread);
    pthread_cancel(udp_thread);
    pthread_join(tcp_thread, NULL);
    pthread_join(udp_thread, NULL);

    return 0;
}