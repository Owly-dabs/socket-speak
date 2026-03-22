#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
#include "directory_manager.h"
#include "uid.h"

/*
Converts the current UID integer to an 8-character hexadecimal string.
The resulting string is stored in the static hex_uid buffer and returned.
*/
static char *uid_int_to_hex(int uid)
{
    sprintf(hex_uid, "%08X", uid);
    hex_uid[8] = '\0'; // Ensure null termination
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
    /* Read uid.txt*/
    FILE *uid_file = open_file_in_user_directory("uid.txt", "r");

    if (uid_file != NULL)
    {
        fscanf(uid_file, "%s", &hex_uid);
        fclose(uid_file);
    }
    else
    {
        int UID = generate_uid();
        /* Write the new UID to uid.txt */
        uid_file = open_file_in_user_directory("uid.txt", "w");
        if (uid_file != NULL)
        {
            char *hex_string = uid_int_to_hex(UID);
            strcpy(hex_uid, hex_string); // Store the hex string in the static buffer
            fprintf(uid_file, "%s", hex_uid);
            fclose(uid_file);
        }
        else
        {
            perror("Error writing UID to file");
            exit(EXIT_FAILURE);
        }
    }
    hex_uid[8] = '\0'; // Ensure null termination
    return hex_uid;
}