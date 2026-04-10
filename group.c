/*
Group Communications Functions
Handle group communications for the project, including accepting connections and managing group membership.
There can only exist one group per server, and a group can have a maximum of 10 members. Each member is identified by their UID, which is an 8-character hexadecimal string.
*/
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "uid.h"
#include "group.h"

/* For Group Server */
static Group current_group = {0};

void add_member_to_group(int socket, const char *uid)
{
    GroupMember *new_member;
    if (current_group.member_count >= MAX_GROUP_MEMBERS)
    {
        fprintf(stderr, "Group is full. Cannot add more members.\n");
        return;
    }

    /* Add new member to the group */
    new_member = &current_group.members[current_group.member_count];
    new_member->socket = socket;
    strncpy(new_member->uid, uid, UID_LENGTH);
    new_member->uid[UID_LENGTH] = '\0'; /* Ensure null termination */

    current_group.member_count++;
}