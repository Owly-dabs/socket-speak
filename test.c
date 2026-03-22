/* All the testing code */
#include <stdio.h>
#include <string.h>
#include "directory_manager.h"
#include "uid.h"

void test_directory_manager_default(void)
{
    char *dir;
    printf("Testing default user directory...\n");

    dir = get_user_directory();
    printf("User directory: %s\n", dir);
}

void test_directory_manager(void)
{
    char *dir;
    printf("Testing custom user directory...\n");

    set_program_username("testuser");
    dir = get_user_directory();
    printf("User directory: %s\n", dir);
}

void test_create_file_in_user_directory(void)
{
    char *dir;
    const char *filename = "testfile.txt";
    char filepath[512];
    FILE *file;

    printf("Testing file creation in custom user directory...\n");

    set_program_username("testuser");
    dir = get_user_directory();
    printf("User directory: %s\n", dir);

    strcpy(filepath, get_user_directory());
    strcat(filepath, filename);

    file = open_file_in_user_directory(filename, "w");
    if (file == NULL)
    {
        printf("Error opening file: %s\n", filepath);
    }
    fclose(file);
    remove(filepath);
}

void test_uid(void)
{
    char *uid;
    printf("Testing UID generation...\n");

    uid = get_uid();
    printf("Generated UID: %s\n", uid);
}

int main()
{
    test_directory_manager_default();
    test_directory_manager();
    test_create_file_in_user_directory();
    test_uid();
    return 0;
}