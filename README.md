# SocketSpeak

## Setup
1. Compile the client and server files
```
$ gcc -o client ./client.c
$ gcc -o server ./server.c
```
2. Find out the local IP address of your own device (or use the local IP address of another device as the server)
    You can use the localAddr.c file to do so:
    ```
    $ gcc -o localAddr ./localAddr.c
    $ ./localAddr
    ```
    Find the line that starts with `wlp` or `wlan`. The corresponding IP address is your local IP address.

## Usage
1. Run the server first
```
$ ./server
```
It should print the following:
```
Accepting new client
```

2. Run the client
```
$ ./client <server_local_ip_addr>
```
For example, `./client 192.168.1.12`
