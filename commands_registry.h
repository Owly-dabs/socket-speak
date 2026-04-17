/* commands_registry.h */
#ifndef commands_registry_H
#define commands_registry_H

/* Update with your command codes */
typedef enum
{
    LMP_MSG = 0x01,
    LMP_NICK = 0x02,
    LMP_ACK = 0x03,
    LMP_MEOW = 0x10,
    LMP_ERROR = 0xFF,
    LMP_UID = 0x04,
    LMP_GRP_OBJ = 0x80
} LMPCode;

void init_commands(void);

#endif /* commands_registry_H */