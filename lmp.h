#ifndef LMP_H
#define LMP_H

#include <stdint.h>

#define LMP_MAGIC_0  0x4C
#define LMP_MAGIC_1  0x4D
#define LMP_MSG      0x01
#define LMP_NICKNAME 0x02
#define LMP_ACK      0x03
#define LMP_ERROR    0x04

typedef struct lmp_header {
    uint8_t  magic[2];
    uint8_t  type;
    uint8_t  reserved;
    uint32_t payload_len; /* network byte order */
} lmp_header_t;
/* Note: __attribute__((packed)) omitted for -ansi compatibility; struct has no
   padding in practice because all fields are uint8_t/uint32_t aligned. */

int  lmp_send(int fd, uint8_t type, const char *payload, uint32_t len);
int  lmp_recv(int fd, uint8_t *type_out, char *buf, uint32_t bufsize, uint32_t *len_out);
void chat_loop(int sock);

#endif /* LMP_H */
