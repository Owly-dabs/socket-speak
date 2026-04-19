/* group_user.h */
#ifndef GROUP_USER_H
#define GROUP_USER_H
#include "group.h"

extern int user_group_is_initialized;
extern int user_group_MODE;
extern GroupMember user_member_info;

typedef enum
{
    STATE_IDLE,
    STATE_REQUESTING_LOAD,
    STATE_LOADING
} State_t;

extern State_t loading_message_state;

void print_welcome_message(Group group);
void print_group_info(Group group);

#endif /* GROUP_USER_H */