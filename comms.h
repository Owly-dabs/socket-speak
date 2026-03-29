/* Include Broadcast and Accept Sockets */
#ifndef COMMS_H
#define COMMS_H
#include <netinet/in.h>
#include <arpa/inet.h>

#define BROADCAST_PORT 8082
#define PORT 8081
#define MAXLINE 1024
#define BROADCAST_IP "255.255.255.255"
#define LISTEN_TIMEOUT_SEC 5
static const int SOCKOPT = 1;

struct in_addr get_my_ip(void);
void broadcast(void);
int listen_for_connection(void);
void listen_for_broadcast(struct sockaddr_in *source_addr, char *buffer, size_t buffer_size);
int connect_to(const struct in_addr *target_ip);

#endif /* COMMS_H */