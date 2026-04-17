/* group_commands.h */
#ifndef GROUP_COMMANDS_H
#define GROUP_COMMANDS_H
#include "lmp.h"

CommandResult grp_obj_send(uint8_t code, const char *args, LMPContext *ctx);

void group_commands_init(void);

#endif /* GROUP_COMMANDS_H */