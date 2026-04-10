#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include "group.h"
#include "group_server_comms.h"

/*
Running Command:
gcc test_group_server.c group_server_comms.c group.c uid.c directory_manager.c -o test_group_server && ./test_group_server
 */
int main()
{
    char *group_UID = "12345678";    /* Example UID (8 hex characters) */
    char *group_name = "Test Group"; /* Example group name */
    pthread_t p;
    GroupInfo group_info;
    strcpy(group_info.group_UID, group_UID);
    group_info.group_UID[UID_LENGTH] = '\0'; /* Ensure null termination */
    strcpy(group_info.group_name, group_name);
    group_info.group_name[GROUP_NAME_MAX_LEN - 1] = '\0'; /* Ensure null termination */
    p = server_create_UDP_reply_thread(&group_info);
    sleep(50);         /* Keep the main thread alive for a while to allow testing */
    pthread_cancel(p); /* Clean up the thread after testing */
    return 0;
}