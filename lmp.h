#ifndef LMP_H
#define LMP_H

#include <stdint.h>
#include <stddef.h>
#include "group.h"

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
    char peer_nick[NICKNAME_MAX_LEN];
    char my_nick[NICKNAME_MAX_LEN];
    char peer_ip[64];
    char my_uid[9];
    char peer_uid[9];
    volatile int peer_uid_ready; /* set by receiver when peer_uid handshake is done */
    char peer_dir[256];
    char history_path[256];
    int history_loaded;
} LMPContext;

typedef CommandResult (*SendHandler)(uint8_t code, const char *args, LMPContext *ctx);
typedef CommandResult (*RecvHandler)(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx);

void register_command(uint8_t code, const char *name, SendHandler send, RecvHandler recv);
CommandResult dispatch_send(const char *line, LMPContext *ctx);
CommandResult dispatch_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx);

ssize_t read_all(int fd, void *buf, size_t n);
void print_prompt(LMPContext *ctx);
void strip_newline(char *s);

int lmp_send(int fd, uint8_t type, const char *payload, uint32_t len);
int lmp_recv(int fd, uint8_t *type_out, char *buf, uint32_t bufsize, uint32_t *len_out);
void chat(int sock, const char *role);

void chat_loop(int sock, const char *peer_ip, const char *history_path);
void chat_loop_user(int sock, const char *server_ip, const char *history_path);

int get_peer_ip(int sockfd, char *ip_str, size_t ip_str_len);
void connection_handler(int sock);

int lmp_send_uid(LMPContext *ctx);
void lmp_history_prepare(LMPContext *ctx);
void lmp_history_load(LMPContext *ctx);
int lmp_history_append(LMPContext *ctx, const char *speaker, const char *message);
int lmp_save_nick(const char *nick);
int lmp_save_peer_nick(const char *peer_uid, const char *nick);
int lmp_load_peer_nick(const char *peer_uid, char *nick, size_t nick_size);

#endif /* LMP_H */
