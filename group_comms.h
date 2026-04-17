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

typedef struct
{
    int sock; /* Server sock */
    char my_nick[NICKNAME_MAX_LEN];
    char group_ip[64]; /* LMPContext::peer_ip */
    char my_uid[9];
    char group_uid[9];   /* LMPContext::peer_uid */
    char group_dir[256]; /* LMPContext::peer_dir */
    char history_path[256];
    int history_loaded;
    Group group;
} GroupContext;

#endif /* GROUP_H */