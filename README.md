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
- I/O 멀티플렉싱: `poll()` 또는 `epoll()`

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

담당 영역 기준으로 브랜치명을 정합니다.

```
feature/<담당>-<기능>
fix/<담당>-<버그요약>
```

- `<담당>`: `network` | `parser` | `channel` | `common` | `docs`
- `<기능>` / `<버그요약>`: kebab-case, 동사보다는 대상 중심으로 짧게

예시:
- `feature/network-poll-loop`
- `feature/parser-message-parsing`
- `feature/channel-join-part`
- `fix/network-fd-leak-on-disconnect`

규칙:
- `main`은 항상 빌드/동작 가능한 상태로 유지 (직접 push 금지)
- 모든 변경은 `feature/*` 또는 `fix/*` 브랜치에서 작업 후 **PR을 통해서만** `main`에 병합
- PR은 **본인 외 최소 1인 리뷰/승인** 후 머지 (담당 영역이 겹치는 부분은 관련자 전원 리뷰)
- 머지 방식은 `Squash and merge`로 통일 (커밋 히스토리 정리, 이력 추적 용이)
- 머지된 브랜치는 삭제 (원격/로컬 모두 정리)
- `common/` 변경이 포함된 PR은 3인 전원 리뷰 필수

## Commit Convention

<!-- 팀 합의 후 확정. 아래는 권장안 -->

```
<type>(<담당>): <description>
```

- `<담당>`: `network` | `parser` | `channel` | `common` | `docs` | `chore`
- `<description>`: 무엇을 했는지 간결하게, 현재형 동사로 시작 (예: "추가", "수정", "제거")

| type | 의미 | 예시 |
|---|---|---|
| `feat` | 기능 추가 | `feat(parser): PRIVMSG 파싱 구현` |
| `fix` | 버그 수정 | `fix(network): poll 이벤트 누락 수정` |
| `refactor` | 동작 변경 없는 코드 개선 | `refactor(channel): Channel::kick 중복 로직 정리` |
| `test` | 테스트 추가/수정 | `test(parser): NICK 커맨드 파싱 테스트 추가` |
| `docs` | 문서만 수정 | `docs: 브랜치 컨벤션 예시 추가` |
| `chore` | 빌드/설정 등 잡무 | `chore: .gitignore에 빌드 산출물 추가` |

- 커밋은 작게, 하나의 논리적 변경 단위로 (리뷰/롤백 용이)
- 제목은 50자 내외로, 본문이 필요하면 한 줄 띄우고 "왜" 바꿨는지 서술

## Issue / Task Tracking

<!-- 팀 합의 후 확정 -->

- GitHub Issues로 작업 단위 관리, PR에 `Closes #이슈번호` 연결
- 라벨 예시: `network` / `parser` / `channel` / `common` / `bug` / `week1` `week2` `week3`


## References

- UNIX Network Programming Vol.1 (Stevens, 3rd Ed.)
- [RFC 1459 - IRC Protocol](https://datatracker.ietf.org/doc/html/rfc1459)
- [RFC 2812 - IRC Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812)