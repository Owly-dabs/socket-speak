#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>
#include "user_manager.h"
#include "lmp.h"

#define PORT 8081

int main(int argc, char const *argv[])
{
    int server_fd, new_socket, pid;
    int opt = 1;
    struct sockaddr_in address;
    socklen_t addrlen = sizeof(address);
    UserInformation *user_info;

    /* Init user information */
    user_info = init_user_information();
    printf("User UID: %s\n", user_info->uid);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        perror("setsockopt SO_REUSEADDR failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* Single-session by design: accept one client, then close the listening socket.
       Re-run the server to accept a new session. */
    printf("Accepting new client\n");
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0)
    {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    if ((pid = fork()) == -1)
    { /* parent process */
        close(new_socket);
        close(server_fd);
        return -1;
    }
    else if (pid > 0)
    {                     /* parent process */
        close(server_fd); /* parent doesn't need the listener */
        chat(new_socket, "server");
        close(new_socket);
    }
    else if (pid == 0)
    {                      /* child process */
        close(new_socket); /* child doesn't need the client socket */
        connection_handler(server_fd);
        close(server_fd);
    }
    return 0;
}
