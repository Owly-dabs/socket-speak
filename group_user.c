/*
group_user.c
Dedicated for functions which only user (client) side needs, such as printing welcome message using group information sent by server.
*/
#include <stdio.h>
#include <string.h>
#include "group.h"
#include "group_user.h"

State_t loading_message_state = STATE_IDLE;

/* used in /gi (group info) and welcome message */
void print_group_info(Group group)
{
    /*
    [<group name>]
    Current members (<number of members>):
     -<nickname> (UID: <UID>)
     */
    int i;
    const char *group_name = (group.info.group_name[0] != '\0') ? group.info.group_name : "Unknown Group";
    printf("[%s]\nCurrent members (%d):\n", group_name, group.member_count);
    for (i = 0; i < group.member_count; i++)
    {
        const char *nickname = (group.members[i].nickname[0] != '\0') ? group.members[i].nickname : "Unknown";
        const char *uid = (group.members[i].uid[0] != '\0') ? group.members[i].uid : "Unknown";
        printf("- %s (UID: %s)\n", nickname, uid);
    }
}

/* Print welcome message */
void print_welcome_message(Group group)
{
    /*
    Message Format:
    Welcome to group [<group name>]
    Current members (<number of members>):
    - <nickname> (UID: <UID>)
    */
    printf("Welcome to group ");
    print_group_info(group);
}