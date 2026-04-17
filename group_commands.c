#include <stdio.h>
#include <string.h>
#include "lmp.h"
#include "group.h"
#include "commands_registry.h"

extern Group current_group;
extern Group user_group;

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
    printf("[Server]: New group saved. Members: %d\n", user_group.member_count);
    return COMMAND_SUCCESS;
}

void group_commands_init(void)
{
    register_command(LMP_GRP_OBJ, "grpobj", grp_obj_send, grp_obj_recv);
}