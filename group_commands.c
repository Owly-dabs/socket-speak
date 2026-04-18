#include <stdio.h>
#include <string.h>
#include "lmp.h"
#include "group.h"
#include "commands_registry.h"
#include "group_user.h"
#include "group_commands.h"

extern Group current_group;
extern Group user_group;
GroupMember user_member_info;
int user_group_is_initialized;

/* GRP */
CommandResult grp_obj_send(uint8_t code, const char *args, LMPContext *ctx)
{
    char *payload = (char *)&current_group;
    uint32_t payload_len = sizeof(current_group);
    if (lmp_send(ctx->sock, code, payload, payload_len) < 0)
        return COMMAND_ERROR;
    return COMMAND_SUCCESS;
}

static CommandResult grp_obj_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    user_group = *(Group *)buf;
    if (user_group_is_initialized == 0)
    {
        user_group_is_initialized = 1;
        print_welcome_message(user_group);
    }
    else
    {
        printf("[System]: Group information has been updated, type /gi to see the updated group information\n");
    }
    return COMMAND_SUCCESS;
}

/*
Print group information
Command: /gi
*/
static CommandResult grp_info_send(uint8_t code, const char *args, LMPContext *ctx)
{
    print_group_info(user_group);

    return COMMAND_SUCCESS;
}

/* [LMP_GRP_INIT_MEMBER] Send member information to the group server */
CommandResult user_grp_init_send(const int sock)
{
    if (lmp_send(sock, LMP_GRP_INIT_MEMBER, (char *)&user_member_info, sizeof(user_member_info)) < 0)
        return COMMAND_ERROR;
    return COMMAND_SUCCESS;
}

void group_commands_init(void)
{
    register_command(LMP_GRP_OBJ, "grpobj", grp_obj_send, grp_obj_recv);
    register_command(LMP_GRP_INFO, "gi", grp_info_send, NULL);
    register_command(LMP_GRP_INIT_MEMBER, "initmember", NULL, NULL);
}