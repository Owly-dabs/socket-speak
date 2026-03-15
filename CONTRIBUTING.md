# LMP Command Contribution Guide

## Overview

Each command lives in its own `.c` file. You never need to touch `lmp.c` — you only need to create your file and register it in `server.c`/`client.c`.
---

## Step 1 — Create your command file

Create `mycommand_command.c`:

```c
#include "lmp.h"
#include <stdio.h>
#include <string.h>

/* Called when the user types /mycommand <args> */
static CommandResult mycommand_send(uint8_t code, const char *args, LMPContext *ctx) {
    lmp_send(ctx->sock, code, args, (uint32_t)strlen(args));
    return COMMAND_SUCCESS;
}

/* Called when a LMP_MYCOMMAND packet is received */
static CommandResult mycommand_recv(uint8_t code, const char *buf, uint32_t len, LMPContext *ctx) {
    printf("[mycommand]: %s\n", buf);
    return COMMAND_SUCCESS;
}

void mycommand_init(void) {
    register_command(0xXX, "mycommand", mycommand_send, mycommand_recv);
}
```

If your command is receive-only or send-only, pass `NULL` for the unused handler:

```c
register_command(0xXX, "mycommand", NULL, mycommand_recv);  /* receive only */
register_command(0xXX, "mycommand", mycommand_send, NULL);  /* send only    */
```

---

## Step 2 — Add your byte code to the enum

In `lmp.h`, add your code to `LMPCode`:

```c
typedef enum {
    LMP_MSG        = 0x01,
    LMP_NICK       = 0x02,
    LMP_ACK        = 0x03,
    LMP_MYCOMMAND  = 0xXX,  /* <-- add yours here */
    LMP_ERROR      = 0xFF
} LMPCode;
```

Pick an unused byte code. `0xFF` is reserved for `LMP_ERROR`.

---

## Step 3 — Register in `server.c` and `client.c`

Declare and call your init function in  `server.c` and `client.c`:

```c
#include "lmp.h"

void mycommand_init(void);   /* <-- add this */

int main(int argc, char *argv[]) {
    ...
    ...
    mycommand_init();        /* <-- add this */
    chat_loop(sock);
    ...
    ...
    return 0;
}
```

---

## Step 4 — Add to the build

Add your file to the compile command in `.vscode/tasks.json`:

For `server.c`:
```bash
gcc -ansi -pedantic server.c lmp.c base_commands.c -lpthread -o ./bin/server && ./bin/server
```

For `client.c`:
```bash
gcc -ansi -pedantic client.c lmp.c base_commands.c -lpthread -o ./bin/client && ./bin/client ${input:ipAddress}
```

---

## Rules

- **One file per command** — `mycommand_command.c` contains only the handlers for `mycommand`
- **Avoid editing `commands.c`** — the handler (aka, the dispatcher) logic is managed here and will affect all other commands
- **Byte codes are unique** — check the `LMPCode` enum before picking one
- **`0xFF` is reserved** — it is permanently assigned to `LMP_ERROR`
- **Return the right result** — return `COMMAND_SUCCESS` on success, `COMMAND_ERROR` on failure, never `COMMAND_UNRECOGNIZED` (that is for the dispatcher only)
- **Keep handlers `static`** — your send/recv functions should never be visible outside your file, only the `_init` function is public
