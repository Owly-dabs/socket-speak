#include "lmp.h"

#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <pthread.h>

#define MAX_COMMANDS 32
#define MAX_NAME_LEN 32

typedef struct {
    uint8_t     code;
    char        name[MAX_NAME_LEN];
    SendHandler send;
    RecvHandler recv;
} CommandEntry;

static CommandEntry command_table[MAX_COMMANDS];
static size_t       command_count = 0;

void register_command(uint8_t code, const char *name, SendHandler send, RecvHandler recv) {
    size_t i;
    size_t name_len;

    if (!name) {
        fprintf(stderr, "register_command: cannot register command with NULL name (code=%u)\n", (unsigned)code);
        return;
    }

    if (command_count >= MAX_COMMANDS) {
        fprintf(stderr, "register_command: table full, cannot register '%s' (code=%u)\n", name, (unsigned)code);
        return;
    }

    name_len = strlen(name);
    if (name_len >= MAX_NAME_LEN) {
        fprintf(stderr,
                "register_command: name '%s' too long (length=%zu, max=%d), not registering (code=%u)\n",
                name, name_len, MAX_NAME_LEN - 1, (unsigned)code);
        return;
    }

    /* Reject duplicate code or name to keep dispatch behavior unambiguous. */
    for (i = 0; i < command_count; i++) {
        if (command_table[i].code == code) {
            fprintf(stderr,
                    "register_command: duplicate command code %u for name '%s' (already used by '%s')\n",
                    (unsigned)code, name, command_table[i].name);
            return;
        }
        if (strcmp(command_table[i].name, name) == 0) {
            fprintf(stderr,
                    "register_command: duplicate command name '%s' (already used with code %u)\n",
                    name, (unsigned)command_table[i].code);
            return;
        }
    }

    command_table[command_count].code = code;
    command_table[command_count].send = send;
    command_table[command_count].recv = recv;
    /* name_len < MAX_NAME_LEN has been validated, so this cannot truncate. */
    strncpy(command_table[command_count].name, name, MAX_NAME_LEN - 1);
    command_table[command_count].name[MAX_NAME_LEN - 1] = '\0';
    command_count++;
}

CommandResult dispatch_send(const char *line, LMPContext *ctx) {
    char buf[256];
    size_t i;
    char *name;
    char *args;
    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    name = strtok(buf, " ");
    if (!name) return COMMAND_UNRECOGNIZED;

    args = strtok(NULL, "");
    if (!args) args = "";

    for (i = 0; i < command_count; i++) {
        if (strcmp(command_table[i].name, name) == 0) {
            if (!command_table[i].send) return COMMAND_ERROR;
            return command_table[i].send(command_table[i].code, args, ctx);
        }
    }
    return COMMAND_UNRECOGNIZED;
}

CommandResult dispatch_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx) {
    size_t i;
    for (i = 0; i < command_count; i++) {
        if (command_table[i].code == code) {
            if (!command_table[i].recv) return COMMAND_ERROR;
            return command_table[i].recv(code, buf, len, ctx);
        }
    }
    return COMMAND_UNRECOGNIZED;
}

static int read_all(int fd, void *buf, size_t n) {
    size_t total = 0;
    char *p = (char *)buf;
    ssize_t r;
    while (total < n) {
        r = read(fd, p + total, n - total);
        if (r <= 0) return -1;
        total += (size_t)r;
    }
    return 0;
}

static void print_prompt() {
    printf("\r\033[K[You]: ");
    fflush(stdout);
}

static void strip_newline(char *s) {
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n') s[len - 1] = '\0';
}

static void *receiver(void *arg) {
    LMPContext *ctx = (LMPContext *)arg;
    uint8_t  type;
    char     buf[4096];
    uint32_t len;

    while (lmp_recv(ctx->sock, &type, buf, sizeof(buf), &len) == 0) {
        printf("\r\033[K"); 
        if (dispatch_recv(type, buf, len, ctx) == COMMAND_UNRECOGNIZED)
            printf("[warning]: unrecognized message type 0x%02X\n", type);
        print_prompt();
    }

    printf("*** Peer disconnected.\n");
    return NULL;
}

void chat_loop(int sock) {
    pthread_t  recv_thread;
    char       line[1024];
    LMPContext ctx;
    ctx.sock = sock;

    strncpy(ctx.my_nick,   "me",   sizeof(ctx.my_nick)   - 1);
    strncpy(ctx.peer_nick, "peer", sizeof(ctx.peer_nick) - 1);
    ctx.my_nick  [sizeof(ctx.my_nick)   - 1] = '\0';
    ctx.peer_nick[sizeof(ctx.peer_nick) - 1] = '\0';

    pthread_create(&recv_thread, NULL, receiver, &ctx);

    while (1) {
        print_prompt();
        if (!fgets(line, sizeof(line), stdin)) break;
        strip_newline(line);

        if (line[0] == '/') {
            switch (dispatch_send(line + 1, &ctx)) {
                case COMMAND_SUCCESS:      break;
                case COMMAND_ERROR:        printf("Error executing '%s'\n",        line); break;
                case COMMAND_UNRECOGNIZED: printf("Unrecognized command '%s'\n",   line); break;
            }
        } else {
            lmp_send(ctx.sock, LMP_MSG, line, (uint32_t)strlen(line));
        }
    }

    shutdown(sock, SHUT_WR);
    pthread_join(recv_thread, NULL);
}

int lmp_send(int fd, uint8_t type, const char *payload, uint32_t len) {
    lmp_header_t hdr;
    hdr.magic[0] = LMP_MAGIC_0;
    hdr.magic[1] = LMP_MAGIC_1;
    hdr.type = type;
    hdr.reserved = 0x00;
    hdr.payload_len = htonl(len);

    if (send(fd, &hdr, sizeof(hdr), 0) < 0) return -1;
    if (len > 0 && send(fd, payload, len, 0) < 0) return -1;
    return 0;
}

int lmp_recv(int fd, uint8_t *type_out, char *buf, uint32_t bufsize, uint32_t *len_out) {
    lmp_header_t hdr;
    uint32_t plen;

    if (read_all(fd, &hdr, sizeof(hdr)) < 0) return -1;
    if (hdr.magic[0] != LMP_MAGIC_0 || hdr.magic[1] != LMP_MAGIC_1) return -1;

    plen = ntohl(hdr.payload_len);
    if (plen >= bufsize) return -1;

    if (plen > 0 && read_all(fd, buf, plen) < 0) return -1;
    buf[plen] = '\0';

    *type_out = hdr.type;
    *len_out = plen;
    return 0;
}
