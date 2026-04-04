# Socket Speak Copilot Instructions

## Project Intent
- Build a peer chat bootstrap flow over UDP broadcast plus TCP chat transport.
- Every user runs the same entrypoint program: ./bin/main.
- Discovery is symmetric and role is decided at runtime.

## Expected Runtime Flow
1. A user starts main.
2. Program sends one UDP broadcast message containing its local IPv4.
3. Program immediately listens for TCP connection attempts on port 8081 with a 5 second timeout.
4. If a TCP connection arrives in time:
	 - This instance acts as chat client role inside app logic.
	 - It starts chat on the accepted socket.
5. If timeout expires:
	 - This instance switches to "listening for UDP broadcast" mode on port 8082.
	 - On receiving a broadcast from another user, it actively TCP-connects to that sender on port 8081.
	 - This instance then acts as chat server role inside app logic.

Use this behavior as the source of truth when generating code, tests, or docs.

## Networking Contract
- UDP broadcast:
	- Address: 255.255.255.255
	- Port: 8082
	- Payload: sender IPv4 string
- TCP chat transport:
	- Port: 8081
- Timeout for passive accept phase: 5 seconds
- Reuse socket options already used in current code unless asked otherwise.

## Important Source Files
- main.c: orchestration for broadcast, timeout branch, and role selection.
- comms.c / comms.h: broadcast(), listen_for_connection(), listen_for_broadcast(), connect_to().
- lmp.c / lmp.h: framed message protocol and chat loop.
- commands_registry.c / base_commands.c / meow_commands.c: command handlers and dispatch.
- Makefile: canonical build targets and flags.

## Build And Run Guidance
- Prefer Makefile targets over ad hoc compile commands.
- Default compile style is ANSI C with pedantic checks.
- Keep compatibility with:
	- CFLAGS: -ansi -pedantic
	- Link flags: -lpthread

## Coding Rules For Suggestions
- Do not introduce C99+ features that violate -ansi builds.
	- Avoid declarations in for loop initializers.
	- Avoid // comments in C source; use /* ... */.
	- Avoid mixed declarations/statements where this breaks ANSI style.
- Keep function signatures in headers and implementations consistent.
- Preserve existing protocol constants and wire compatibility unless explicitly requested.
- Prefer small focused changes in existing modules over introducing new layers.

## Behavior Safety Checks
When suggesting networking changes, verify all of the following remain true:
1. Every main instance broadcasts first.
2. Accept timeout path is exactly the mode-switch trigger.
3. Timeout path listens for UDP and then initiates TCP connect back.
4. Chat starts only after TCP socket establishment.
5. Ports stay consistent between sender and listener paths.
6. Any discovery-phase socket timeout (SO_RCVTIMEO) must not remain active on the established chat socket.

## Testing Expectations
- At minimum, validate build success with make.
- For runtime checks, validate with two terminals/machines:
	- First run: starts, broadcasts, then times out and waits for UDP.
	- Second run: broadcasts while first is waiting; first detects it and connects.
	- Chat should start on both sides with stable send/receive behavior.

## Documentation Expectations
- Keep README and comments consistent with actual main handshake flow.
- If behavior changes, update both user flow docs and constant descriptions.
