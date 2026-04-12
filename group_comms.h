#ifndef GROUP_COMMS_H
#define GROUP_COMMS_H
/* Group Communications Functions */
#include "group.h"
#include <arpa/inet.h>

#define GROUP_UDP_PORT 8090
#define GROUP_TCP_PORT 8085
#define MAGIC_WORD "DISCOVER_SERVER_V1"
#define GROUP_DISCOVERY_MAGIC 0x4744 /* "GD" in ASCII */

/* Group Discovery Reply Message Structure for server to reply to clients */
typedef struct
{
    uint8_t magic[2]; /* 0x47 0x44 = "GD" for Group Discovery */
    uint8_t status;   /* 0 = ready, 1 = full, etc */
    GroupInfo info;
} GroupDiscoveryReplyMsg;

#endif /* GROUP_H */