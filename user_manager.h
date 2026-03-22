/* user_manager.h */
#ifndef USER_MANAGER_H
#define USER_MANAGER_H

typedef struct
{
    char uid[9]; /* 8 characters for UID + null terminator */
} UserInformation;
extern UserInformation user;

UserInformation *init_user_information(void);
void flush_user_information(void);

#endif /* USER_MANAGER_H */