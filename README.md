# ft_irc

7.11 깃 세팅 도중 디렉토리를 어떻게 짜야 하는지를 문제로
작성된 Readme입니다. <br>
현재 내용은 참고만 하시고, 함께 모일 때 추가 모의를 진행할 예정입니다!

## Build

```bash
make
./ircserv <port> <password>
```

## Requirements

- C++98
- 컴파일 플래그: `-Wall -Wextra -Werror`
- I/O 멀티플렉싱: `poll()` 또는 `epoll`

## Team & Ownership

| 역할 | 담당 | 소유 디렉토리 | 소유 클래스 | 소유 커맨드 |
|---|---|---|---|---|
| Network | TBD | `server/`, `client/` | Server, PollManager, Socket, Client | - |
| Parser | TBD | `parser/` | Parser, Message | PASS, NICK, USER, PING, PONG, QUIT |
| Channel/Cmd | TBD | `channel/` | Channel | JOIN, PART, TOPIC, MODE, KICK, INVITE, PRIVMSG, NOTICE |

> `Client`는 Network가 생성/관리하지만 Parser(등록 상태), Channel(소속 채널)도 필드를 참조함.
> `common/`은 3인 합의 후 최대한 변경 최소화.

## Directory Structure

#### 미러링 구조
`include/`와 `srcs/`는 동일한 하위 구조로 미러링됨 (`include/X` ↔ `srcs/X`).<br>
- 아래는 추천 구조입니다.(클래스 만들 때 참고)
- 디렉토리 세부 수정은 16일 회의에서 조정하시죠!!


```
include/
├── server/                    ← Network 담당 전용 구역
│   ├── Server.hpp
│   ├── PollManager.hpp
│   └── Socket.hpp
├── client/
│   └── Client.hpp              ← Network 소유, 전원 참조
├── parser/                    ← Parser 담당 전용 구역
│   ├── Parser.hpp
│   ├── Message.hpp
│   └── commands/
│       ├── Pass.hpp
│       ├── Nick.hpp
│       ├── User.hpp
│       ├── Ping.hpp
│       ├── Pong.hpp
│       └── Quit.hpp
├── channel/                   ← Channel/Cmd 담당 전용 구역 (PRIVMSG/NOTICE 포함)
│   ├── Channel.hpp
│   └── commands/
│       ├── Join.hpp
│       ├── Part.hpp
│       ├── Topic.hpp
│       ├── Mode.hpp
│       ├── Kick.hpp
│       ├── Invite.hpp
│       ├── Privmsg.hpp
│       └── Notice.hpp
└── common/                    ← 3인 합의 필요, 최대한 변경 최소화
    ├── ICommand.hpp            (커맨드 인터페이스, 순수가상함수 — .cpp 없음)
    ├── Replies.hpp             (RFC 규격 응답 코드 상수 — .cpp 없음)
    └── Utils.hpp

srcs/  (include/ 와 미러링, 단 .cpp가 불필요한 헤더 전용 파일은 제외 *)
├── main.cpp
├── server/
│   ├── Server.cpp
│   ├── PollManager.cpp
│   └── Socket.cpp
├── client/
│   └── Client.cpp
├── parser/
│   ├── Parser.cpp
│   ├── Message.cpp
│   └── commands/
│       └── (Pass.cpp, Nick.cpp, User.cpp, Ping.cpp, Pong.cpp, Quit.cpp)
├── channel/
│   ├── Channel.cpp
│   └── commands/
│       └── (Join.cpp, Part.cpp, Topic.cpp, Mode.cpp, Kick.cpp, Invite.cpp, Privmsg.cpp, Notice.cpp)
└── common/
    └── Utils.cpp

* ICommand.hpp, Replies.hpp는 인터페이스/상수 전용 헤더라 대응 .cpp 없음
```

새 커맨드 추가 시: `include/<owner>/commands/`와 `srcs/<owner>/commands/`에 동일한 이름으로 `.hpp`/`.cpp` 쌍 생성.

## Roadmap (3 weeks)

- [ ] **Week 1** — 인터페이스 합의 (Server/Client/Channel/ICommand/Message 시그니처) + 개별 스켈레톤
- [ ] **Week 2** — 병렬 구현 + 1차 통합 (`nc` 다중 클라이언트 테스트)
- [ ] **Week 3** — 엣지 케이스, 메모리/fd 누수 점검, defense 준비

세부 일정: [PLAN.md](./PLAN.md) <!-- 필요 시 별도 문서로 분리 -->

## Testing

- 로컬: `nc <host> <port>`
- 표준 클라이언트: irssi, HexChat 등으로 defense 전 검증

## Branch Convention

<!-- 팀 합의 후 채우기: 예) feature/<owner>-<기능>, PR 필수 여부 등 -->

## References

- UNIX Network Programming Vol.1 (Stevens, 3rd Ed.)
- [RFC 1459 - IRC Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 - IRC Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)