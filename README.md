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

> **Note:** The server is single-session by design. It accepts exactly one client connection per invocation and exits when that client disconnects. To start a new session, run the server again.

2. Run the client
```
$ ./client <server_local_ip_addr>
```
For example, `./client 192.168.1.12`

## LAN Messaging Protocol (LMP)

+--------+--------+----------+------------------+------------------+
| Magic  | Type   | Reserved | Payload Length   | Payload          |
| 2 bytes| 1 byte | 1 byte   | 4 bytes (uint32) | N bytes          |
+--------+--------+----------+------------------+------------------+

Magic -- Set-in-stone 2 bytes, 0x4C4D, to signify start of payload
Type -- type of payload (msg, nickname, etc.)
Reserved -- for future use
Payload length -- 4 bytes gives up to 4GB



## Testing

### Commands

```bash
gcc test.c directory_manager.c -ansi -pedantic && ./a.out
```