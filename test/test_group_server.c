#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "group.h"
#include "group_server_comms.h"

static void *tcp_listen_entry(void *arg)
{
    (void)arg;
    group_server_TCP_listen();
    return NULL;
}

static void print_connected_members(void)
{
    int i;

    printf("Connected members (%d):\n", connection_count);
    for (i = 0; i < connection_count; i++)
    {
        printf("  [%d] uid=%s nickname=%s socket=%d\n",
               i + 1,
               group_connections[i].member.uid,
               group_connections[i].member.nickname,
               group_connections[i].tcp_socket);
    }
}

/*
Running Command:
gcc test_group_server.c group_server_comms.c group.c uid.c directory_manager.c -o test_group_server && ./test_group_server
 */
int main()
{
    char *group_UID = "12345678";    /* Example UID (8 hex characters) */
    char *group_name = "Test Group"; /* Example group name */
    pthread_t udp_thread;
    pthread_t tcp_thread;
    char command[128];
    int create_status;

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
            print_connected_members();
        }
        else if (strcmp(command, "/close") == 0)
        {
            int i;

            for (i = 0; i < connection_count; i++)
            {
                if (group_connections[i].tcp_socket > 0)
                {
                    close(group_connections[i].tcp_socket);
                }
            }

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