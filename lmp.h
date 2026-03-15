#ifndef LMP_H
#define LMP_H

#include <stdint.h>

#define LMP_MAGIC_0  0x4C
#define LMP_MAGIC_1  0x4D

typedef struct lmp_header {
    uint8_t  magic[2];
    uint8_t  type;
    uint8_t  reserved;
    uint32_t payload_len; /* network byte order */
} lmp_header_t;
/* Note: __attribute__((packed)) omitted for -ansi compatibility; struct has no
   padding in practice because all fields are uint8_t/uint32_t aligned. */

typedef enum {
    LMP_MSG     = 0x01,
    LMP_NICK    = 0x02,
    LMP_ACK     = 0x03,
    LMP_ERROR   = 0xFF
} LMPCode;

typedef enum {
    COMMAND_SUCCESS,
    COMMAND_ERROR,
    COMMAND_UNRECOGNIZED
} CommandResult;

typedef struct {
    int  sock;
    char peer_nick[64];
    char my_nick[64];     
} LMPContext;

typedef CommandResult (*SendHandler)(uint8_t code, const char *args, LMPContext *ctx);
typedef CommandResult (*RecvHandler)(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx);

void            register_command(uint8_t code, const char *name, SendHandler send, RecvHandler recv);
CommandResult   dispatch_send(const char *line, LMPContext *ctx);
CommandResult   dispatch_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx);

int  lmp_send(int fd, uint8_t type, const char *payload, uint32_t len);
int  lmp_recv(int fd, uint8_t *type_out, char *buf, uint32_t bufsize, uint32_t *len_out);
void chat_loop(int sock);

#endif /* LMP_H */
