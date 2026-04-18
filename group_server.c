#include <stdio.h>
#include <string.h>
#include "group.h"
#include "directory_manager.h"
#include "group_server.h"
#include "uid.h"

extern Group current_group;
char group_dir[512];

int save_message_to_history(const char *uid, const char *message)
{
    char history_file_path[512];
    FILE *history_file;
    size_t group_dir_len;
    size_t history_file_len;
    size_t total_len;

    MessageFormat message_format;
    memset(&message_format, 0, sizeof(MessageFormat));

    group_dir_len = strlen(group_dir);
    history_file_len = strlen(GROUP_HISTORY_FILE);
    total_len = group_dir_len + history_file_len;

    if (total_len >= sizeof(history_file_path))
    {
        fprintf(stderr, "History file path too long\n");
        return -1;
    }

    memcpy(history_file_path, group_dir, group_dir_len);
    memcpy(history_file_path + group_dir_len, GROUP_HISTORY_FILE, history_file_len);
    history_file_path[total_len] = '\0';

    history_file = fopen(history_file_path, "a");
    if (!history_file)
    {
        perror("Error opening history file");
        return -1;
    }

    /* Check the uid is length of UID_LENGTH */
    if (strlen(uid) != UID_LENGTH)
    {
        fprintf(stderr, "Invalid UID length\n");
        return -1;
    }

    strncpy(message_format.uid, uid, UID_LENGTH);

    /* Check the message length less than 1024 - UID_LENGTH */

    if (strlen(message) >= sizeof(message_format.message))
    {
        fprintf(stderr, "Message too long\n");
        return -1;
    }

    strncpy(message_format.message, message, sizeof(message_format.message) - 1);

    fprintf(history_file, "%s\n", (char *)&message_format);
    fclose(history_file);
    return 0;
}

void init_group_server(const char *group_UID)
{
    init_group_server_directory(group_UID, group_dir, sizeof(group_dir));
    /* add "/" to the end of group_dir */
    if (group_dir[strlen(group_dir) - 1] != '/')
    {
        strcat(group_dir, "/");
    }
}