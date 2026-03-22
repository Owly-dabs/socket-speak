/* uid.h */
#ifndef UID_H
#define UID_H

static const int DEFAULT_UID = 1000;
static int UID = DEFAULT_UID;
static char hex_uid[9] = {0}; // 8 characters for hex + null terminator

static char *uid_int_to_hex(int uid);
int generate_uid(void);
char *get_uid(void);

#endif /* UID_H */