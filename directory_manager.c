#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include "directory_manager.h"

typedef enum
{
    MADE_DIRECTORY_FALSE,
    MADE_DIRECTORY_TRUE
} md;

static char pusername[256] = "default/";
static char user_directory[512] = {0};
static md made_directory = MADE_DIRECTORY_FALSE;

/* Expands ROOT_DIRECTORY when it starts with "~/" into an absolute HOME path. */
static void get_root_directory(char *buffer, size_t buffer_size)
{
    buffer[0] = '\0';

    if (strncmp(ROOT_DIRECTORY, "~/", 2) == 0)
    {
        const char *home = getenv("HOME");
        if (home != NULL && home[0] != '\0')
        {
            strncat(buffer, home, buffer_size - 1);
            strncat(buffer, "/", buffer_size - strlen(buffer) - 1);
            strncat(buffer, &ROOT_DIRECTORY[2], buffer_size - strlen(buffer) - 1);
            return;
        }
    }

    strncat(buffer, ROOT_DIRECTORY, buffer_size - 1);
}

/**
 * GENERATED CODE
 * Checks if a given path exists and is a directory.
 * @param pathname The path to check.
 * @return 1 if it is a directory, 0 otherwise (including if it doesn't exist or is a file).
 */
static void make_directory(const char *pathname)
{
    char command[512] = "mkdir -p ";
    strcat(command, pathname);
    if (system(command) == 0)
    {
        made_directory = MADE_DIRECTORY_TRUE;
    }
    else
    {
        perror("Error make_directory");
        exit(EXIT_FAILURE);
    }
}

/*
Updates the user directory based on the current program username
Do not call this function directly; use set_program_username() instead to ensure the username is set correctly.
*/
static void set_user_directory(void)
{
    char root_directory[512];
    get_root_directory(root_directory, sizeof(root_directory));

    user_directory[0] = '\0';
    strcpy(user_directory, root_directory);
    strcat(user_directory, pusername);
    made_directory = MADE_DIRECTORY_FALSE;
}

void init_user_directory(void)
{
    if (made_directory == MADE_DIRECTORY_FALSE)
    {
        make_directory(user_directory);
    }
}

char *get_user_directory(void)
{
    if (user_directory[0] == '\0')
    {
        set_user_directory();
    }
    init_user_directory();
    return user_directory;
}

/* Sets the username for the program and updates the user directory */
void set_program_username(const char *username)
{
    strcpy(pusername, username);
    strcat(pusername, "/");
    set_user_directory();
}

FILE *open_file_in_user_directory(const char *filename, const char *mode)
{
    FILE *file;
    char filepath[512];
    strcpy(filepath, get_user_directory());
    strcat(filepath, filename);

    file = fopen(filepath, mode);
    return file;
}

void init_group_server_directory(const char *uid, char *group_dir, size_t group_dir_size)
{
    char root_directory[512];
    size_t root_len;
    size_t uid_len;
    size_t prefix_len;

    get_root_directory(root_directory, sizeof(root_directory));

    root_len = strlen(root_directory);
    uid_len = strlen(uid);
    prefix_len = 8; /* "/server/" */

    if (root_len + prefix_len + uid_len + 1 > group_dir_size)
    {
        fprintf(stderr, "group directory path too long\n");
        exit(EXIT_FAILURE);
    }

    memcpy(group_dir, root_directory, root_len);
    memcpy(group_dir + root_len, "/server/", prefix_len);
    memcpy(group_dir + root_len + prefix_len, uid, uid_len + 1);

    make_directory(group_dir);
}