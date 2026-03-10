#include <unistd.h>
#include <stdio.h>
#include <sys/socket.h>
#include <stdlib.h>
#include <netinet/in.h> /* contains constants and structures needed for internet domain addresses. */
#include <string.h>
#define PORT 8081

int main(int argc, char const *argv[])
{
    /**
     * server_fd and new_socket are file descriptors, i.e. array subscripts into the file descriptor table. These two variables store the values returned by the socket system call and the accept system call.
     * valread is the return value for the read() and write() calls; i.e. it contains the number of characters read or written.
     **/
    int server_fd, new_socket, valread;
    /**
     * A sockaddr_in is a structure containing an internet address. This structure is defined in <netinet/in.h>. Here is the definition:
        struct sockaddr_in {
        short   sin_family;
        u_short sin_port;
        struct  in_addr sin_addr;
        char    sin_zero[8];
        };
    */
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    char buffer[1024] = {0}; /* buffer used to read data from socket */
    char *hello = "Hello from server";

    /* Creating socket file descriptor, using socket system call */
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    /* Forcefully attaching socket to the port 8080 */
    if (bind(server_fd, (struct sockaddr *)&address,
             sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 3) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    /* multiple clients accepted */
    for (;;)
    {
        int pid;
        printf("Accepting new client\n");
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        if ((pid = fork()) == -1) /* fork fails */
        {
            close(new_socket);
            continue;
        }

        else if (pid > 0) /* parent process */
        {
            close(new_socket);
            continue;
        }
        else if (pid == 0) /* child process */
        {
            valread = read(new_socket, buffer, 1024);
            printf("%s\n", buffer);
            send(new_socket, hello, strlen(hello), 0);
            printf("Hello message sent\n");
            return 0;
        }
    }
    return 0;
}