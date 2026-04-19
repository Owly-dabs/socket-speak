#include <stdio.h>
#include <string.h>
#include "lmp.h"
#include "commands_registry.h"
#include "group_user.h"

int user_group_MODE;

/* MSG */
static CommandResult msg_send(uint8_t code, const char *args, LMPContext *ctx)
{
    if (lmp_send(ctx->sock, code, args, (uint32_t)strlen(args)) < 0)
        return COMMAND_ERROR;
    lmp_history_append(ctx, ctx->my_nick, args);
    return COMMAND_SUCCESS;
}

static CommandResult msg_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    printf("[%s]: %s\n", ctx->peer_nick, buf);
    lmp_history_append(ctx, ctx->peer_nick, buf);
    return COMMAND_SUCCESS;
}

/* NICK */
static CommandResult nick_send(uint8_t code, const char *args, LMPContext *ctx)
{
    if (user_group_MODE == 0) /* One to one connection */
    {
        strncpy(ctx->my_nick, args, sizeof(ctx->my_nick) - 1);
        ctx->my_nick[sizeof(ctx->my_nick) - 1] = '\0';
        lmp_save_nick(ctx->my_nick); /* Save nickname to disk so it persists across sessions */
    }
    if (lmp_send(ctx->sock, code, args, (uint32_t)strlen(args)) < 0)
        return COMMAND_ERROR;
    return COMMAND_SUCCESS;
}

static CommandResult nick_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    strncpy(ctx->peer_nick, buf, sizeof(ctx->peer_nick) - 1);
    ctx->peer_nick[sizeof(ctx->peer_nick) - 1] = '\0';
    lmp_save_peer_nick(ctx->peer_uid, ctx->peer_nick);
    printf("*** Peer is now known as: %s\n", ctx->peer_nick);
    if (lmp_send(ctx->sock, LMP_ACK, "nickname ack", 12) < 0)
        return COMMAND_ERROR;
    return COMMAND_SUCCESS;
}

/* ACK */
static CommandResult ack_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    printf("[System]: %s\n", buf);
    return COMMAND_SUCCESS;
}

/* ERROR */
static CommandResult error_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    printf("[error]: %s\n", buf);
    return COMMAND_SUCCESS;
}

/* UID */
static CommandResult uid_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    uint32_t copy_len;

    (void)code;

    if (ctx == NULL || buf == NULL)
        return COMMAND_ERROR;

    copy_len = len;
    if (copy_len > 8)
        copy_len = 8;

    memcpy(ctx->peer_uid, buf, copy_len);
    ctx->peer_uid[copy_len] = '\0';

    lmp_history_prepare(ctx);

    /* Load saved peer nickname if it exists */
    lmp_load_peer_nick(ctx->peer_uid, ctx->peer_nick, sizeof(ctx->peer_nick));

    if (ctx->history_loaded == 0)
    {
        lmp_history_load(ctx);
        ctx->history_loaded = 1;
    }
    return COMMAND_SUCCESS;
}

void base_commands_init(void)
{
    register_command(LMP_MSG, "msg", msg_send, msg_recv);
    register_command(LMP_NICK, "nick", nick_send, nick_recv);
    register_command(LMP_ACK, "ack", NULL, ack_recv);
    register_command(LMP_ERROR, "error", NULL, error_recv);
    register_command(LMP_UID, "uid", NULL, uid_recv);
}