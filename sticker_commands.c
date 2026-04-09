#include "lmp.h"
#include "commands_registry.h"
#include <stdio.h>
#include <string.h>

/* Called when the user types /mycommand <args> */
static CommandResult sticker_send(uint8_t code, const char *args, LMPContext *ctx) {
    lmp_send(ctx->sock, code, args, (uint32_t)strlen(args)); 
    return COMMAND_SUCCESS;
}

/* Called when a LMP_MYCOMMAND packet is received */
static CommandResult sticker_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx) {
    printf("[sticker]: %s\n", buf);
    return COMMAND_SUCCESS;
}

void sticker_commands_init(void) {
    register_command(LMP_STICKERS, "sticker", sticker_send, sticker_recv);
}