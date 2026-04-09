#include "commands_registry.h"
#include "base_commands.h"
/* Add #include directives for any other required headers */
#include "meow_commands.h"
#include "sticker_commands.h"

void init_commands(void)
{
    base_commands_init();

    /* Append any additional command initializations here */
    meow_commands_init();
    sticker_commands_init();
}