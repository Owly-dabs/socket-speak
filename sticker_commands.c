#include "lmp.h"
#include "commands_registry.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define MAX_NAME_LEN 32
#define MAX_STICKERS 64
#define MAX_STICKER_LEN 2048
#define ESCAPE 27

typedef struct
{
    char name[MAX_NAME_LEN];
    char sticker[MAX_STICKER_LEN];
} StickerEntry;

static StickerEntry sticker_array[MAX_STICKERS];
static size_t sticker_count = 0;
static struct termios orig_termios;

static void raw_mode_enable(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);  /* disable echo and line buffering */
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

static void raw_mode_disable(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

char *read_multiline_overlay(void) {
    size_t buf_size = 1024;
    size_t total_len = 0;
    char *buffer = malloc(buf_size);
    int c;
    char *new_buf;

    if (!buffer) return NULL;
    buffer[0] = '\0';

    raw_mode_enable();

    /* Save cursor position and screen, then clear */
    printf("\033[?47h");    /* save screen */
    printf("\033[2J");      /* clear screen */
    printf("\033[H");       /* cursor to top-left */
    printf("Enter message (ESC to finish):\n\n");
    fflush(stdout);

    while ((c = getchar()) != ESCAPE) {
        if (c == EOF) break;

        /* Handle backspace */
        if (c == 127 || c == '\b') {
            if (total_len > 0) {
                total_len--;
                buffer[total_len] = '\0';
                printf("\b \b");
                fflush(stdout);
            }
            continue;
        }

        /* Grow buffer if needed */
        if (total_len + 2 > buf_size) {
            buf_size *= 2;
            new_buf = realloc(buffer, buf_size);
            if (!new_buf) { free(buffer); raw_mode_disable(); return NULL; }
            buffer = new_buf;
        }

        buffer[total_len++] = (char)c;
        buffer[total_len] = '\0';
        putchar(c);
        fflush(stdout);
    }

    /* Restore screen */
    printf("\033[?47l");    /* restore screen */
    fflush(stdout);

    raw_mode_disable();
    return buffer;  /* caller must free() */
}


void create_sticker(char *name) {
    char *user_sticker;
    user_sticker = read_multiline_overlay();
    strcpy(sticker_array[sticker_count].sticker, user_sticker);
    strcpy(sticker_array[sticker_count].name, name);
    sticker_count++;
    printf("Sticker %s created!\n", name);
}

char *get_sticker(char *name) {
    int i;
    for (i=0; i < sticker_count; i++) {
        if (strcmp(sticker_array[i].name, name) == 0) {
            return sticker_array[i].sticker;
        }
    }
    return NULL;
}

/* Called when the user types /mycommand <args> */
static CommandResult sticker_send(uint8_t code, const char *args, LMPContext *ctx) {
    char *args_copy;
    char *command;
    char *sticker;
    char *name;

    args_copy = (char *)malloc(strlen(args) + 1);
    strcpy(args_copy, args);
    command = strtok(args_copy, " ");
    if (command == NULL) {
        free(args_copy);
        printf("No command found!\n");
        return COMMAND_ERROR;
    }
    if (strcmp(command, "create") == 0) {
        name = strtok(NULL, " ");
        if (name == NULL) {
            free(args_copy);
            printf("No command found!\n");
            return COMMAND_ERROR;
        }
        create_sticker(name);
        printf("Sticker %s created!", name);
    } else if (strcmp(command, "send") == 0) {
        name = strtok(NULL, " ");
        if (name == NULL) {
            free(args_copy);
            printf("No command found!\n");
            return COMMAND_ERROR;
        }
        sticker = get_sticker(name);
        if (sticker == NULL) {
            printf("Sticker not found!\n");
        } else {
            printf("%s\n", sticker);
            lmp_send(ctx->sock, code, sticker, (uint32_t)strlen(sticker));
        }
    } else {
        printf("Sticker command %s not found!", command);
    }
    free(args_copy);
    return COMMAND_SUCCESS;
}

/* Called when a LMP_MYCOMMAND packet is received */
static CommandResult sticker_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx) {
    printf("[%s]: Sent a sticker\n%s\n", ctx->peer_nick, buf);
    return COMMAND_SUCCESS;
}

void sticker_commands_init(void) {
    register_command(LMP_STICKERS, "sticker", sticker_send, sticker_recv);
}
