# SocketSpeak

SocketSpeak is a CLI messaging app written in C for local-area-network peer chat. It supports peer-to-peer one-on-one conversations, group chat, ASCII-art stickers, and a synchronous Hangman minigame. The project is designed to stay lightweight and dependency-free beyond a C compiler, `make`, and pthreads.

## Features

- Peer-to-peer auto-discovery over UDP broadcast for one-on-one chat.
- Group chat through a central group server.
- Persistent storage for usernames, chat history, and stickers under `~/lmp/<username>/`.
- ASCII-art sticker creation, preview, and sending.
- Group-only Hangman gameplay.
- Tested on Linux (Debian) and macOS.

## Prerequisites

- GCC or Clang
- `make`
- A POSIX-like environment with pthread support

The project is compiled in ANSI C mode with pedantic checks enabled through the Makefile.

## Build

```bash
make
```

This builds the main binaries in `bin/`:

- `bin/main` - One-on-One Chat
- `bin/gserver` - Group Chat
- `bin/gclient` - Group Chat

It also builds the test binaries under `bin/`.


## Usage

### One-on-One Chat (P2P)

Start either peer with the main binary:

```bash
./bin/main [-u <username>]
```

The `-u` flag sets the program username used for persistent storage. If omitted, the program uses `default` as program username.

Runtime flow:

1. Each peer broadcasts its local IPv4 address over UDP to `255.255.255.255:8082`.
2. The program waits up to 5 seconds for an incoming TCP connection on port `8081`.
3. If a connection arrives, that instance acts as the client side inside the application.
4. If the accept times out, the instance switches to UDP discovery mode and waits for another peer to broadcast.
5. When a broadcast is received, it connects back over TCP to port `8081` and acts as the server side inside the application.

### Group Chat (Group)

Start the group server:

```bash
./bin/gserver
```

Connect a group client:

```bash
./bin/gclient
```

The group client broadcasts a UDP discovery signal, waits for a server response, and then connects over TCP.

## Commands

Once connected, plain text is sent as a message. Commands are prefixed with `/`.

| Command | Description | Mode |
| --- | --- | --- |
| plain text | Send a plain-text message | P2P, Group |
| `/msg <text>` | Explicitly send a message | P2P, Group |
| `/nick <name>` | Set your nickname and persist it | P2P, Group |
| `/sticker <action> ...` | Create, send, list, or preview ASCII-art stickers | P2P |
| `/gi` | Display group info, including member nicknames and UIDs | Group |
| `/load` | Request and display group message history | Group |
| `/hangman <action>` | Start, join, or exit the Hangman game | Group |
| `/meow <text>` | Demo command | P2P, Group |


## Stickers

Stickers can be created, listed, previewed, and sent with the `/sticker` command in P2P chat only.

```bash
/sticker create <sticker_name>
/sticker send <sticker_name>
/sticker list
/sticker preview <sticker_name>
```

- `create` opens a multiline terminal editor for drawing the sticker.
- Press `ESC` to finish editing and save the sticker.
- Stickers are stored in `~/lmp/<username>/saved_stickers.txt`.
- `send` the sticker to other peers in the chat.
- `preview` shows the sticker locally without sending it.

## Hangman

Hangman is available in group chat only.

```bash
/hangman start
/hangman join
/hangman exit
```

- `start` initializes a new game.
- `join` joins an active game.
- `exit` leaves the game and returns to normal messaging.

## LAN Messaging Protocol

SocketSpeak uses a custom binary framing protocol called LMP. Each frame is an 8-byte header followed by a variable-length payload.

| Field | Size | Description |
| --- | --- | --- |
| Magic | 2 bytes | Fixed value `0x4C4D` (`LM`) |
| Type | 1 byte | Command or message type |
| Reserved | 1 byte | Reserved for future use |
| Payload length | 4 bytes | Big-endian `uint32` payload size |
| Payload | N bytes | Message body |

The payload length supports messages up to 4 GB.

## Storage

### User data (P2P and Group Client)

Client-side user data is stored under `~/lmp/<username>/`, including:

- `uid.txt` for the local user UID
- `nick.txt` for the current nickname
- `saved_stickers.txt` for sticker definitions (P2P sticker feature)
- `peers/<peer_uid>/nick.txt` for a peer's saved nickname (P2P)
- `peers/<peer_uid>/history.txt` for one-on-one chat history with that peer (P2P)

This makes user-specific state persistent across sessions.

### Server data (Group)

Group chat history is stored on the group server under `~/lmp/server/<group_uid>/`:

- `history.txt` for chat history