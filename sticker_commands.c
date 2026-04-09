#include "lmp.h"
#include "commands_registry.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MAX_NAME_LEN 32
#define MAX_STICKERS 64
#define MAX_STICKER_LEN 2048

typedef struct
{
    char name[MAX_NAME_LEN];
    char sticker[MAX_STICKER_LEN];
} StickerEntry;

static StickerEntry sticker_array[MAX_STICKERS];
static size_t sticker_count = 0;

void create_sticker(char *name) {
    strcpy(sticker_array[sticker_count].sticker, "----------\n---test---\n---aaaa----\n----------");
    strcpy(sticker_array[sticker_count].name, name);
    sticker_count++;
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
    printf("[%s]: \n%s\n", ctx->peer_nick, buf);
    return COMMAND_SUCCESS;
}

void sticker_commands_init(void) {
    register_command(LMP_STICKERS, "sticker", sticker_send, sticker_recv);
}
