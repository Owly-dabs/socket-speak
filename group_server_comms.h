/* group_server_comms.h */
#ifndef GROUP_SERVER_COMMS_H
#define GROUP_SERVER_COMMS_H
#include <pthread.h>
#include "group.h"

#define MAX_GROUP_CONNECTIONS (MAX_GROUP_MEMBERS + 5) /* Allow some buffer for extra connections */

/* User link list */
typedef struct UserNode
{
    char uid[UID_LENGTH + 1];
    struct UserNode *next;
} UserNode;

void add_user(UserNode **head, const char *uid);    /* Add user to the link list */
void remove_user(UserNode **head, const char *uid); /* Remove user from the link list */
void free_users(UserNode **head);                   /* Free all users from the link list */
int user_in_list(UserNode *head, const char *uid);  /* Check if user is in the link list */
void print_user_list(UserNode *head);               /* Print all users in the link list, for debugging */

pthread_t server_create_UDP_reply_thread(void);
void group_server_UDP_reply(GroupInfo *group_info);
int group_server_TCP_listen(void);

#endif /* GROUP_SERVER_COMMS_H */