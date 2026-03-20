#include <stdio.h>
#include <string.h>
#include "lmp.h"
#include "commands_registry.h"

static CommandResult meow_send(uint8_t code, const char *args, LMPContext *ctx)
{
    printf("meow %s\n", args);
    return COMMAND_SUCCESS;
}

void meow_commands_init(void)
{
    register_command(LMP_MEOW, "meow", meow_send, NULL);
}