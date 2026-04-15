#include "lmp.h"

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include "commands_registry.h"
#include "directory_manager.h"
#include "uid.h"

#define MAX_COMMANDS 32
#define MAX_NAME_LEN 32

typedef struct
{
    uint8_t code;
    char name[MAX_NAME_LEN];
    SendHandler send;
    RecvHandler recv;
} CommandEntry;

static CommandEntry command_table[MAX_COMMANDS];
static size_t command_count = 0;

#define HISTORY_DIR ".history"

void register_command(uint8_t code, const char *name, SendHandler send, RecvHandler recv)
{
    size_t i;
    size_t name_len;

    if (!name)
    {
        fprintf(stderr, "register_command: cannot register command with NULL name (code=%u)\n", (unsigned)code);
        return;
    }

    if (command_count >= MAX_COMMANDS)
    {
        fprintf(stderr, "register_command: table full, cannot register '%s' (code=%u)\n", name, (unsigned)code);
        return;
    }

    name_len = strlen(name);
    if (name_len >= MAX_NAME_LEN)
    {
        fprintf(stderr,
                "register_command: name '%s' too long (length=%lu, max=%d), not registering (code=%u)\n",
                name, (unsigned long)name_len, MAX_NAME_LEN - 1, (unsigned)code);
        return;
    }

    /* Reject duplicate code or name to keep dispatch behavior unambiguous. */
    for (i = 0; i < command_count; i++)
    {
        if (command_table[i].code == code)
        {
            fprintf(stderr,
                    "register_command: duplicate command code %u for name '%s' (already used by '%s')\n",
                    (unsigned)code, name, command_table[i].name);
            return;
        }
        if (strcmp(command_table[i].name, name) == 0)
        {
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

CommandResult dispatch_send(const char *line, LMPContext *ctx)
{
    char buf[256];
    size_t i;
    char *name;
    char *args;

    if (strlen(line) >= sizeof(buf))
    {
        fprintf(stderr, "dispatch_send: command line too long (max %d characters)\n",
                (int)(sizeof(buf) - 1));
        return COMMAND_ERROR;
    }

    strncpy(buf, line, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    name = strtok(buf, " ");
    if (!name)
        return COMMAND_UNRECOGNIZED;

    args = strtok(NULL, "");
    if (!args)
        args = "";

    for (i = 0; i < command_count; i++)
    {
        if (strcmp(command_table[i].name, name) == 0)
        {
            if (!command_table[i].send)
                return COMMAND_ERROR;
            return command_table[i].send(command_table[i].code, args, ctx);
        }
    }
    return COMMAND_UNRECOGNIZED;
}

CommandResult dispatch_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx)
{
    size_t i;
    for (i = 0; i < command_count; i++)
    {
        if (command_table[i].code == code)
        {
            if (!command_table[i].recv)
                return COMMAND_ERROR;
            return command_table[i].recv(code, buf, len, ctx);
        }
    }
    return COMMAND_UNRECOGNIZED;
}

static ssize_t read_all(int fd, void *buf, size_t n)
{
    size_t total = 0;
    char *p = (char *)buf;
    ssize_t r;

    while (total < n)
    {
        r = read(fd, p + total, n - total);
        if (r < 0)
        {
            if (errno == EINTR)
                continue; /* retry on interrupt */
            return -1;    /* real error */
        }
        if (r == 0)
        {
            /* EOF - return number of bytes actually read */
            return (ssize_t)total;
        }
        total += (size_t)r;
    }
    return (ssize_t)total;
}

static void print_prompt(LMPContext *ctx)
{
    printf("\r\033[K[%s]: ", ctx->my_nick);
    fflush(stdout);
}

static void strip_newline(char *s)
{
    size_t len = strlen(s);
    if (len > 0 && s[len - 1] == '\n')
        s[len - 1] = '\0';
}

static void *receiver(void *arg)
{
    LMPContext *ctx = (LMPContext *)arg;
    uint8_t type;
    char buf[4096];
    uint32_t len;

    while (lmp_recv(ctx->sock, &type, buf, sizeof(buf), &len) == 0)
    {
        printf("\r\033[K");
        if (dispatch_recv(type, buf, len, ctx) == COMMAND_UNRECOGNIZED)
            printf("[warning]: unrecognized message type 0x%02X\n", type);
        print_prompt(ctx);
    }

    printf("*** Peer disconnected.\n");
    return NULL;
}

int lmp_send_uid(LMPContext *ctx)
{
    if (ctx == NULL || ctx->my_uid[0] == '\0')
        return -1;

    return lmp_send(ctx->sock, LMP_UID, ctx->my_uid, (uint32_t)strlen(ctx->my_uid));
}

/*helper function to create a directory for lmp_history_prepare*/
static void lmp_make_dir(const char *pathname)
{
    char command[512] = "mkdir -p ";
    strcat(command, pathname);

    if (system(command) != 0)
    {
        perror("mkdir -p failed");
        exit(EXIT_FAILURE);
    }
}

void lmp_history_prepare(LMPContext *ctx)
{
    if (ctx == NULL)
        return;

    ctx->peer_dir[0] = '\0';
    ctx->history_path[0] = '\0';

    if (ctx->peer_uid[0] == '\0')
        return;

    strcpy(ctx->peer_dir, get_user_directory());
    strcat(ctx->peer_dir, "peers/");
    lmp_make_dir(ctx->peer_dir);

    strcat(ctx->peer_dir, ctx->peer_uid);
    strcat(ctx->peer_dir, "/");
    lmp_make_dir(ctx->peer_dir);

    strcpy(ctx->history_path, ctx->peer_dir);
    strcat(ctx->history_path, "history.txt");
}

void lmp_history_load(LMPContext *ctx)
{
    FILE *fp;
    char line[512];

    if (ctx == NULL || ctx->history_path[0] == '\0')
        return;

    fp = fopen(ctx->history_path, "r");
    if (fp == NULL)
        return;

    printf("\r\033[K"); /* Clear the current prompt line before printing history */
    while (fgets(line, sizeof(line), fp) != NULL)
    {
        printf("%s", line);
    }

    fclose(fp);
}

int lmp_history_append(LMPContext *ctx, const char *speaker, const char *message)
{
    FILE *fp;

    if (ctx == NULL || speaker == NULL || message == NULL)
        return -1;
    if (ctx->history_path[0] == '\0')
        return -1;

    fp = fopen(ctx->history_path, "a");
    if (fp == NULL)
        return -1;

    fprintf(fp, "[%s]: %s\n", speaker, message);
    fclose(fp);
    return 0;
}

int lmp_save_nick(const char *nick)
{
    FILE *fp = open_file_in_user_directory("nick.txt", "w");
    if (fp == NULL)
        return -1;
    fprintf(fp, "%s", nick);
    fclose(fp);
    return 0;
}

int lmp_save_peer_nick(const char *peer_uid, const char *nick)
{
    FILE *fp;
    char filename[32];
    sprintf(filename, "peers/%s/nick.txt", peer_uid);
    fp = open_file_in_user_directory(filename, "w");
    if (fp == NULL)
        return -1;
    fprintf(fp, "%s", nick);
    fclose(fp);
    return 0;
}

int lmp_load_peer_nick(const char *peer_uid, char *nick, size_t nick_size)
{
    FILE *fp;
    char filename[32];
    sprintf(filename, "peers/%s/nick.txt", peer_uid);
    fp = open_file_in_user_directory(filename, "r");
    if (fp == NULL)
        return -1;
    fscanf(fp, "%63s", nick);
    fclose(fp);
    return 0;
}

/* Middleware function to handle chat functionality */
/* int sock: socket file descriptor */
/* const char *role: role of the user (e.g., "server", "client") */
void chat(int sock, const char *role)
{
    char peer_ip[INET_ADDRSTRLEN];

    (void)role;

    if (get_peer_ip(sock, peer_ip, sizeof(peer_ip)) == 0)
        printf("Connected from IP: %s\n", peer_ip);
    else
        strncpy(peer_ip, "unknown_peer", sizeof(peer_ip) - 1);

    peer_ip[sizeof(peer_ip) - 1] = '\0';

    init_commands(); /* Initialize all commands */
    chat_loop(sock, peer_ip, "");
}

/* Actual chat loop implementation */
void chat_loop(int sock, const char *peer_ip, const char *history_path)
{
    pthread_t recv_thread;
    char line[1024];
    LMPContext ctx;
    FILE *fp;
    ctx.sock = sock;

    strncpy(ctx.my_nick, "You", sizeof(ctx.my_nick) - 1);
    strncpy(ctx.peer_nick, "Peer", sizeof(ctx.peer_nick) - 1);
    ctx.my_nick[sizeof(ctx.my_nick) - 1] = '\0';
    ctx.peer_nick[sizeof(ctx.peer_nick) - 1] = '\0';

    /* Load saved nickname if it exists */
    fp = open_file_in_user_directory("nick.txt", "r");
    if (fp != NULL)
    {
        fscanf(fp, "%63s", ctx.my_nick);
        fclose(fp);
    }

    strncpy(ctx.peer_ip, peer_ip ? peer_ip : "unknown_peer", sizeof(ctx.peer_ip) - 1);
    ctx.peer_ip[sizeof(ctx.peer_ip) - 1] = '\0';

    strncpy(ctx.history_path, history_path ? history_path : "", sizeof(ctx.history_path) - 1);
    ctx.history_path[sizeof(ctx.history_path) - 1] = '\0';

    ctx.peer_uid[0] = '\0';
    ctx.history_loaded = 0;

    strncpy(ctx.my_uid, get_uid(), sizeof(ctx.my_uid) - 1);
    ctx.my_uid[sizeof(ctx.my_uid) - 1] = '\0';

    pthread_create(&recv_thread, NULL, receiver, &ctx);

    /* send my UID once chat starts */
    lmp_send_uid(&ctx);

    while (ctx.peer_uid[0] == '\0')
    {
        /* wait for peer UID before allowing chat */
    }

    while (1)
    {
        print_prompt(&ctx);
        if (!fgets(line, sizeof(line), stdin))
            break;
        strip_newline(line);

        if (line[0] == '/')
        {
            switch (dispatch_send(line + 1, &ctx))
            {
            case COMMAND_SUCCESS:
                break;
            case COMMAND_ERROR:
                printf("Error executing '%s'\n", line);
                break;
            case COMMAND_UNRECOGNIZED:
                printf("Unrecognized command '%s'\n", line);
                break;
            }
        }
        else
        {
            if (lmp_send(ctx.sock, LMP_MSG, line, (uint32_t)strlen(line)) == 0)
                lmp_history_append(&ctx, ctx.my_nick, line);
        }
    }

    shutdown(sock, SHUT_WR);
    pthread_join(recv_thread, NULL);
}

int lmp_send(int fd, uint8_t type, const char *payload, uint32_t len)
{
    lmp_header_t hdr;
    hdr.magic[0] = LMP_MAGIC_0;
    hdr.magic[1] = LMP_MAGIC_1;
    hdr.type = type;
    hdr.reserved = 0x00;
    hdr.payload_len = htonl(len);

    if (send(fd, &hdr, sizeof(hdr), 0) < 0)
        return -1;
    if (len > 0 && send(fd, payload, len, 0) < 0)
        return -1;
    return 0;
}
int lmp_recv(int fd, uint8_t *type_out, char *buf, uint32_t bufsize, uint32_t *len_out)
{
    lmp_header_t hdr;
    uint32_t plen;

    int header_bytes = read_all(fd, &hdr, sizeof(hdr));
    if (header_bytes < 0)
        return -1;
    if (header_bytes == 0)
        return -2; /* temp code for EOF (connection closed)*/

    /* Checking valid header */
    if (hdr.magic[0] != LMP_MAGIC_0 || hdr.magic[1] != LMP_MAGIC_1 || header_bytes < sizeof hdr)
        return -1;

    plen = ntohl(hdr.payload_len);
    if (plen >= bufsize)
        return -1;

    if (plen > 0 && read_all(fd, buf, plen) <= 0)
        return -1;
    buf[plen] = '\0';

    *type_out = hdr.type;
    *len_out = plen;
    return 0;
}

int get_peer_ip(int sockfd, char *ip_str, size_t ip_str_len)
{
    struct sockaddr_storage peer_addr;
    socklen_t peer_addr_len = sizeof(peer_addr);

    if (getpeername(sockfd, (struct sockaddr *)&peer_addr, &peer_addr_len) == -1)
    {
        perror("getpeername failed");
        return -1;
    }

    /* Check address family and convert to string */
    if (peer_addr.ss_family == AF_INET)
    {
        struct sockaddr_in *ipv4 = (struct sockaddr_in *)&peer_addr;
        if (inet_ntop(AF_INET, &(ipv4->sin_addr), ip_str, ip_str_len) == NULL)
        {
            perror("inet_ntop (IPv4) failed");
            return -1;
        }
    }
    else if (peer_addr.ss_family == AF_INET6)
    {
        printf("Connecting with IPv6 address, not supported in this server\n");
        return -1;
    }
    else
    {
        printf("Unknown address family\n");
        return -1;
    }

    return 0;
}

void connection_handler(int sock)
{ /* handles incoming connections */
    int temp_sock;
    struct sockaddr *address;
    socklen_t addrlen = sizeof(address);

    for (;;)
    {
        if ((temp_sock = accept(sock, (struct sockaddr *)&address, &addrlen)) < 0)
        {
            perror("accept: server unable to listen to new connections");
            exit(EXIT_FAILURE);
        };

        if (lmp_send(temp_sock, LMP_ERROR, "server busy", 11) < 0)
        {
            perror("lmp_send");
        }

        close(temp_sock);
    }
}