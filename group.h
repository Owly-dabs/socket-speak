#ifndef GROUP_H
#define GROUP_H
#include <arpa/inet.h>
#include "uid.h"

#define MAX_GROUP_MEMBERS 10
#define GROUP_NAME_MAX_LEN 32

typedef struct GroupMember
{
    int socket;
    char uid[UID_LENGTH + 1]; /* 8 characters for hex + null terminator */
} GroupMember;

typedef struct GroupInfo
{
    char group_UID[UID_LENGTH + 1];      /* 8 characters for hex + null terminator */
    char group_name[GROUP_NAME_MAX_LEN]; /* Group name (max 31 chars + null terminator) */
} GroupInfo;

typedef struct Group
{
    GroupInfo info;
    GroupMember members[MAX_GROUP_MEMBERS]; /* Maximum of 10 members in a group */
    int member_count;
} Group;

void add_member_to_group(int socket, const char *uid);

#endif /* GROUP_H */