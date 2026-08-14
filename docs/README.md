*This project has been created as part of the 42 curriculum by borlee, jooyepar, mjoh.*

# ft_irc

## Description

Created in **August 1988 by Jarkko Oikarinen** at the University of Oulu, Finland, **Internet Relay Chat (IRC)** is one of the earliest and most influential real-time text messaging protocols on the Internet, establishing the foundations for modern communication platforms like Slack and Discord.

**Project Goal**: **ft_irc** is a custom Internet Relay Chat server written from scratch in **C++98**, fully compliant with the **RFC 1459** specification. The primary objective is to implement a robust, single-threaded, non-blocking network server capable of managing multiple concurrent client connections, channels, and real-time message broadcasting using the `poll()` I/O multiplexing mechanism.

### Architecture & Directory Structure

#### 1. Layered Architecture

`ft_irc` is structured with strict separation of concerns across three core layers:

1. **Network Layer (`Server`, `PollManager`, `Socket`, `Client`)**:
   - Manages non-blocking TCP socket lifecycles, I/O multiplexing via `poll()`, incoming stream accumulation, CRLF (`\r\n`) message boundary extraction, and outgoing buffer queues (handling partial sends).
2. **Parser Layer (`Parser`, `Message`)**:
   - Validates prefix, command, parameters, and trailing components.
   - Enforces registration state transitions (`PASS` ➔ `NICK` ➔ `USER`) and dispatches commands.
3. **Channel & Command Layer (`Channel`, `ICommand` handlers)**:
   - Manages channel state, membership, permissions, modes, and executes RFC-compliant numeric reply generation.

#### 2. Directory Structure

```text
.
├── Makefile
├── include/
│   ├── client/
│   │   └── Client.hpp           ← Client state and buffer management
│   ├── server/
│   │   ├── Server.hpp           ← Server main controller
│   │   ├── PollManager.hpp      ← pollfd array and I/O polling manager
│   │   └── Socket.hpp           ← Socket setup and non-blocking wrapper
│   ├── parser/
│   │   ├── Parser.hpp           ← Message parser and command dispatcher
│   │   ├── Message.hpp          ← Parsed IRC message struct
│   │   └── commands/            ← Pass, Nick, User, Ping, Pong, Quit
│   ├── channel/
│   │   ├── Channel.hpp          ← Channel state, modes, and members
│   │   └── commands/            ← Join, Part, Topic, Mode, Kick, Invite
│   └── common/
│       ├── ICommand.hpp         ← Command interface
│       ├── Replies.hpp          ← RFC numeric reply constants
│       └── Utils.hpp            ← Utility helper functions
└── src/
    ├── main.cpp                 ← Program entry point
    ├── client/
    │   └── Client.cpp
    ├── server/
    │   ├── Server.cpp
    │   ├── ServerAuth.cpp
    │   ├── Servercmds.cpp
    │   ├── PollManager.cpp
    │   └── Socket.cpp
    ├── parser/
    │   ├── Parser.cpp
    │   ├── Message.cpp
    │   └── commands/
    ├── channel/
    │   ├── Channel.cpp
    │   └── commands/
    └── common/
        └── Utils.cpp
```

### Features

#### 1. Supported Commands

| Category | Command | Description |
|---|---|---|
| **Connection & Auth** | `PASS` | Authenticates connection with the server password |
| | `NICK` | Sets or updates client nickname (with uniqueness validation) |
| | `USER` | Specifies username and realname to complete client registration |
| | `PING` / `PONG` | Heartbeat mechanism to monitor connection vitality |
| | `QUIT` | Gracefully closes connection and notifies shared channels |
| **Channel Operations** | `JOIN` | Creates or joins a channel (verifies key, limits, and invite status) |
| | `PART` | Leaves specified channel(s) |
| | `TOPIC` | Views or updates the topic of a channel |
| | `MODE` | Views or alters channel modes and operator permissions |
| | `KICK` | Ejects a user from a channel (Operator only) |
| | `INVITE` | Invites a user to an invite-only channel (Operator only) |
| **Messaging** | `PRIVMSG` | Sends private messages to individual users or broadcasts to channels |

#### 2. Supported Channel Modes

- `+i` / `-i`: Set/remove Invite-only channel.
- `+t` / `-t`: Set/remove restrictions of the `TOPIC` command to channel operators.
- `+k` / `-k`: Set/remove channel key (password protection).
- `+o` / `-o`: Grant/revoke channel operator privilege.
- `+l` / `-l`: Set/remove maximum user limit for the channel.

---

## Instructions

### 1. Build

Compile the executable using the provided `Makefile` (`-Wall -Wextra -Werror -std=c++98`):

```bash
make            # Build ircserv executable
make clean      # Remove object and dependency files
make fclean     # Remove all binaries and build artifacts
make re         # Clean rebuild
```

### 2. Run

Launch the server by specifying a listening port (`1` - `65535`) and a network password:

```bash
./ircserv <port> <password>

# Example: Run on port 6667 with password '1234'
./ircserv 6667 1234
```

### 3. Connect & Test

- **Standard IRC Client (Irssi)**:
  ```bash
  # Direct one-line connection (Recommended)
  irssi -c 127.0.0.1 -p 6667 -w 1234 -n mynick
  ```
  Or inside the Irssi shell:
  ```text
  /connect 127.0.0.1 6667 1234
  /nick mynick
  /join #general
  /msg #general Hello, IRC world!
  ```

- **Raw Protocol Testing (Netcat)**:
  ```bash
  nc -C 127.0.0.1 6667
  ```
  Once connected, send the RFC 1459 registration handshake:
  ```text
  PASS 1234
  NICK testuser
  USER testuser 0 * :Test User
  JOIN #lobby
  PRIVMSG #lobby :Hello from Netcat!
  ```


---

## Resources & AI Usage

### References

- [RFC 1459: Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459) — Official specification for IRC architecture, message formats, and core command protocol.
- [RFC 2812: Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812) — Updated protocol specifications for modern client interactions and channel management.

### Use of AI

In compliance with the 42 curriculum requirements, AI tools were transparently utilized for the following tasks:

- **CodeRabbit**:
  - **Code Review**: Automated pull request (PR) reviews for bug detection and code style consistency.
- **AI Assistants**:
  - **RFC Clarification**: Looking up message formats, edge cases, and numeric reply codes in RFC 1459.
  - **Test Script Assistance**: Writing Python test scripts for regression testing.
  - **OOP & C++ Best Practices**: Consulting on object-oriented architecture, C++98 idioms, and modular design principles.
