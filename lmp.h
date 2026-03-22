#ifndef LMP_H
#define LMP_H

#include <stdint.h>
#include <stddef.h>

#define LMP_MAGIC_0 0x4C
#define LMP_MAGIC_1 0x4D

typedef struct lmp_header
{
    uint8_t magic[2];
    uint8_t type;
    uint8_t reserved;
    uint32_t payload_len; /* network byte order */
} lmp_header_t;
/* Note: __attribute__((packed)) omitted for -ansi compatibility; struct has no
   padding in practice because all fields are uint8_t/uint32_t aligned. */

typedef enum
{
    COMMAND_SUCCESS,
    COMMAND_ERROR,
    COMMAND_UNRECOGNIZED
} CommandResult;

typedef struct
{
    int sock;
    char peer_nick[64];
    char my_nick[64];
    char peer_ip[64];
    char history_path[256];
} LMPContext;

typedef CommandResult (*SendHandler)(uint8_t code, const char *args, LMPContext *ctx);
typedef CommandResult (*RecvHandler)(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx);

void register_command(uint8_t code, const char *name, SendHandler send, RecvHandler recv);
CommandResult dispatch_send(const char *line, LMPContext *ctx);
CommandResult dispatch_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx);

int lmp_send(int fd, uint8_t type, const char *payload, uint32_t len);
int lmp_recv(int fd, uint8_t *type_out, char *buf, uint32_t bufsize, uint32_t *len_out);
void chat(int sock, const char *role);
void chat_loop(int sock, const char *peer_ip, const char *history_path);
int get_peer_ip(int sockfd, char *ip_str, size_t ip_str_len);
void connection_handler(int sock);

int lmp_history_ensure_dir(void);
int lmp_history_build_path(const char *peer_ip, char *out_path, size_t out_path_len);
int lmp_history_append(LMPContext *ctx, const char *direction, const char *message);
int lmp_history_print(const char *path);

#endif /* LMP_H */
