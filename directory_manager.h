/* directory_manager.h */
#ifndef DIRECTORY_MANAGER_H
#define DIRECTORY_MANAGER_H
#define ROOT_DIRECTORY "~/lmp/"
#include <stdio.h>

static char pusername[256] = "default/";
static char user_directory[512] = {0};
typedef enum
{
    MADE_DIRECTORY_FALSE,
    MADE_DIRECTORY_TRUE
} md;
static md made_directory = MADE_DIRECTORY_FALSE;

static void get_root_directory(char *buffer, size_t buffer_size);
static void make_directory(const char *pathname);
static void set_user_directory(void);
static void init_user_directory(void);
char *get_user_directory(void);
void set_program_username(const char *pusername);
FILE *open_file_in_user_directory(const char *filename, const char *mode);

#endif /* DIRECTORY_MANAGER_H */