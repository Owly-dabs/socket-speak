/* group_server_comms.h */
#ifndef GROUP_SERVER_COMMS_H
#define GROUP_SERVER_COMMS_H
#include <pthread.h>
#include "group.h"

#define MAX_GROUP_CONNECTIONS (MAX_GROUP_MEMBERS + 5) /* Allow some buffer for extra connections */

/* Array of TCP connections to UID */
typedef struct GroupConnection
{
    GroupMember member; /* Group member information (UID) */
    int tcp_socket;     /* TCP socket file descriptor */
} GroupConnection;

extern GroupConnection group_connections[MAX_GROUP_CONNECTIONS];
extern int connection_count; /* Number of active connections */

pthread_t server_create_UDP_reply_thread();
void group_server_UDP_reply(GroupInfo *group_info);
int group_server_TCP_listen();

#endif /* GROUP_SERVER_COMMS_H */