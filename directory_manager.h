/* directory_manager.h */
#ifndef DIRECTORY_MANAGER_H
#define DIRECTORY_MANAGER_H
#define ROOT_DIRECTORY "~/lmp/"
#include <stdio.h>

char *get_user_directory(void);
void set_program_username(const char *pusername);
FILE *open_file_in_user_directory(const char *filename, const char *mode);
void init_user_directory(void);

#endif /* DIRECTORY_MANAGER_H */