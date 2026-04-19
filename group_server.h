/* group_server.h */
#ifndef GROUP_SERVER_H
#define GROUP_SERVER_H
#define GROUP_HISTORY_FILE "history.txt"

int save_message_to_history(const char *uid, const char *message);
void init_group_server(const char *group_UID);
char *get_history_file_path();

#endif /* GROUP_SERVER_H */