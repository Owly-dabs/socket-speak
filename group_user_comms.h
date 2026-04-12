/* group_user_comms.h */
#ifndef GROUP_USER_COMMS_H
#define GROUP_USER_COMMS_H

#define CLIENT_UDP_GROUP_TIMEOUT 2 /* Timeout in seconds for client UDP discovery */

int user_UDP_to_group_server(struct sockaddr_in *server_addr, GroupDiscoveryReplyMsg *reply);
int user_TCP_to_group_server(struct sockaddr_in *server_addr);

#endif /* GROUP_USER_COMMS_H */