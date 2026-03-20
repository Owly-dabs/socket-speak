#include "lmp.h"
#include <stdio.h>
#include <string.h>

/* MSG */
static CommandResult msg_send(uint8_t code, const char *args, LMPContext *ctx) {
    if (lmp_send(ctx->sock, code, args, (uint32_t)strlen(args)) < 0)
        return COMMAND_ERROR;
    return COMMAND_SUCCESS;
}

static CommandResult msg_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx) {
    printf("[%s]: %s\n", ctx->peer_nick, buf);
    return COMMAND_SUCCESS;
}

/* NICK */
static CommandResult nick_send(uint8_t code, const char *args, LMPContext *ctx) {
    strncpy(ctx->my_nick, args, sizeof(ctx->my_nick) - 1);
    ctx->my_nick[sizeof(ctx->my_nick) - 1] = '\0';
    if (lmp_send(ctx->sock, code, args, (uint32_t)strlen(args)) < 0)
        return COMMAND_ERROR;
    return COMMAND_SUCCESS;
}

static CommandResult nick_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx) {
    strncpy(ctx->peer_nick, buf, sizeof(ctx->peer_nick) - 1);
    ctx->peer_nick[sizeof(ctx->peer_nick) - 1] = '\0';
    printf("*** Peer is now known as: %s\n", ctx->peer_nick);
    if (lmp_send(ctx->sock, LMP_ACK, "nickname ack", 12) < 0)
        return COMMAND_ERROR;
    return COMMAND_SUCCESS;
}

/* ACK */
static CommandResult ack_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx) {
    printf("[system]: %s\n", buf);
    return COMMAND_SUCCESS;
}

/* ERROR */
static CommandResult error_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx) {
    printf("[error]: %s\n", buf);
    return COMMAND_SUCCESS;
}

void base_commands_init(void) {
    register_command(LMP_MSG,   "msg",   msg_send,  msg_recv);
    register_command(LMP_NICK,  "nick",  nick_send, nick_recv);
    register_command(LMP_ACK,   "ack",   NULL,      ack_recv);
    register_command(LMP_ERROR, "error", NULL,      error_recv);
}