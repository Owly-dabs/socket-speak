#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include "directory_manager.h"
#include "uid.h"

static char hex_uid[UID_LENGTH + 1] = {0}; /* 8 characters for hex + null terminator */

static int uid_string_valid(const char *s)
{
    int i;
    if (s == NULL)
        return 0;
    for (i = 0; i < UID_LENGTH; i++)
    {
        if (!isxdigit((unsigned char)s[i]))
            return 0;
    }
    if (s[UID_LENGTH] != '\0')
        return 0;
    return 1;
}

/*
Converts the current UID integer to an 8-character hexadecimal string.
The resulting string is stored in the static hex_uid buffer and returned.
*/
static char *uid_int_to_hex(int uid)
{
    sprintf(hex_uid, "%08X", uid);
    hex_uid[UID_LENGTH] = '\0'; /* Ensure null termination */
    return hex_uid;
}

/* UID between 0x10000000 and INT_MAX  */
int generate_uid(void)
{
    int UID;
    srand((unsigned int)time(NULL));
    UID = (rand() % (INT_MAX - 268435456)) + 268435456;
    return UID;
}

char *get_uid(void)
{
    FILE *uid_file;
    int need_generate = 1;

    memset(hex_uid, 0, sizeof(hex_uid));
    uid_file = open_file_in_user_directory("uid.txt", "r");
    if (uid_file != NULL)
    {
        if (fscanf(uid_file, "%8s", hex_uid) == 1 && uid_string_valid(hex_uid))
            need_generate = 0;
        fclose(uid_file);
    }

    if (need_generate)
    {
        int UID = generate_uid();
        uid_file = open_file_in_user_directory("uid.txt", "w");
        if (uid_file != NULL)
        {
            /* uid_int_to_hex writes into hex_uid; do not strcpy(src,dst) with overlapping buffers */
            (void)uid_int_to_hex(UID);
            fprintf(uid_file, "%s", hex_uid);
            fclose(uid_file);
        }
        else
        {
            perror("Error writing UID to file");
            exit(EXIT_FAILURE);
        }
    }
    hex_uid[UID_LENGTH] = '\0'; /* Ensure null termination */
    return hex_uid;
}