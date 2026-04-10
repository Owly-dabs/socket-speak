/* group_server_comms.h */
#ifndef GROUP_SERVER_COMMS_H
#define GROUP_SERVER_COMMS_H
#include <pthread.h>

pthread_t server_create_UDP_reply_thread(GroupInfo *group_info);
void group_server_UDP_reply(GroupInfo *group_info);

#endif /* GROUP_SERVER_COMMS_H */