#include <string.h>
#include <stdio.h>
#include "uid.h"
#include "user_manager.h"
UserInformation user; /* Global variable to hold user information */

/* Run once when the program starts */
UserInformation *init_user_information(void)
{
    char *uid = get_uid();
    strncpy(user.uid, uid, sizeof(user.uid) - 1);
    user.uid[sizeof(user.uid) - 1] = '\0'; /* Ensure null-termination */
    return &user;
}

void flush_user_information(void)
{
    memset(&user, 0, sizeof(UserInformation));
}