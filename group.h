#ifndef GROUP_H
#define GROUP_H
#include <arpa/inet.h>
#include "uid.h"

#define MAX_GROUP_MEMBERS 10
#define GROUP_NAME_MAX_LEN 32
#define NICKNAME_MAX_LEN 32

typedef struct GroupMember
{
    char uid[UID_LENGTH + 1];        /* 8 characters for hex + null terminator */
    char nickname[NICKNAME_MAX_LEN]; /* Nickname (max 31 chars + null terminator) */
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

extern Group current_group;

typedef struct MessageFormat
{
    char uid[UID_LENGTH];
    char message[1024 - UID_LENGTH];
} MessageFormat; /* Ensure MessageFormat is size of 1024 */

#endif /* GROUP_H */