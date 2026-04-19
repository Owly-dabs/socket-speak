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

/* Initiate load history message from server */
/* Usage: /load */
static CommandResult user_grp_load_msg_send(uint8_t code, const char *args, LMPContext *ctx)
{
    if (loading_message_state == STATE_REQUESTING_LOAD)
    {
        printf("[System]: Already requesting load, please wait...\n");
        return COMMAND_SUCCESS;
    }
    if (loading_message_state == STATE_LOADING)
    {
        printf("[System]: Loading, please wait...\n");
        return COMMAND_SUCCESS;
    }
    printf("[System]: Loading History...\n");
    loading_message_state = STATE_REQUESTING_LOAD;
    if (lmp_send(ctx->sock, LMP_GRP_LOAD_MSG, NULL, 0) < 0)
        return COMMAND_ERROR;
    return COMMAND_SUCCESS;
}

static CommandResult user_grp_load_msg_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    if (loading_message_state == STATE_REQUESTING_LOAD || loading_message_state == STATE_LOADING)
    {
        loading_message_state = STATE_IDLE;
    }
    return COMMAND_SUCCESS;
}

static CommandResult user_grp_loading_msg_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    HistoryFormat *history;
    int i;
    const char *nickname;
    char uid[UID_LENGTH + 1];

    if (loading_message_state == STATE_IDLE)
    {
        printf("[System]: Received unexpected loading message, ignoring...\n");
        return COMMAND_SUCCESS;
    }

    loading_message_state = STATE_LOADING;

    history = (HistoryFormat *)buf;
    nickname = "Unknown";

    /* Copy first UID_LENGTH characters from history->uid and add null terminator */
    strncpy(uid, history->uid, UID_LENGTH);
    uid[UID_LENGTH] = '\0';

    /* if user_member_info uid is the same as the history uid */
    if (strcmp(user_member_info.uid, uid) == 0)
    {
        nickname = "You";
    }
    else
    {
        for (i = 0; i < user_group.member_count; i++)
        {
            if (strcmp(user_group.members[i].uid, uid) == 0)
            {
                nickname = user_group.members[i].nickname;
                break;
            }
        }
    }

    printf("[%s]: %s", nickname, history->message);

    return COMMAND_SUCCESS;
}

/* Start hangman game */
static CommandResult user_grp_hangman_send(uint8_t code, const char *args, LMPContext *ctx)
{
    /* Sanitize input, check if arg is start, join, or exit */
    if (strcmp(args, "start") == 0 || strcmp(args, "join") == 0 || strcmp(args, "exit") == 0)
    {
        if (lmp_send(ctx->sock, code, args, strlen(args)) < 0)
            return COMMAND_ERROR;
    }
    else
    {
        printf("[Hangman System] Invalid command. Please use '/hangman start', '/hangman join', or '/hangman exit'\n");
        return COMMAND_ERROR;
    }
    return COMMAND_SUCCESS;
}

static CommandResult user_grp_hangman_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    char response[1024];
    if (len >= sizeof(response))
    {
        fprintf(stderr, "Received hangman message is too long, ignoring...\n");
        return COMMAND_ERROR;
    }
    memcpy(response, buf, len);
    response[len] = '\0';
    printf("%s\n", response);
    return COMMAND_SUCCESS;
}

void group_commands_init(void)
{
    register_command(LMP_GRP_OBJ, "grpobj", grp_obj_send, grp_obj_recv);
    register_command(LMP_GRP_INFO, "gi", grp_info_send, NULL);
    register_command(LMP_GRP_INIT_MEMBER, "initmember", NULL, NULL);
    register_command(LMP_GRP_LOAD_MSG, "load", user_grp_load_msg_send, user_grp_load_msg_recv);
    register_command(LMP_GRP_LOADING_MSG, "grploadingmsg", NULL, user_grp_loading_msg_recv);
    register_command(LMP_GRP_HANGMAN, "hangman", user_grp_hangman_send, user_grp_hangman_recv);
}