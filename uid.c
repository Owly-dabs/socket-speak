#include <stdlib.h>
#include <time.h>
#include <limits.h>
#include <string.h>
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
    srand((unsigned int)time(NULL));
    UID = (rand() % (INT_MAX - 268435456)) + 268435456;
    return UID;
}

char *get_uid(void)
{
    if (UID == DEFAULT_UID)
    {
        generate_uid();
    }
    return uid_int_to_hex(UID);
}