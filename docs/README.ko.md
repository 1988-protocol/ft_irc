*This project has been created as part of the 42 curriculum by borlee, jooyepar, mjoh.*

# ft_irc

## Description

1988년 8월 핀란드 오울루 대학교의 **Jarkko Oikarinen**이 개발한 **IRC(Internet Relay Chat)**는 인터넷 역사상 가장 초기이자 막대한 영향을 끼친 실시간 텍스트 통신 프로토콜 중 하나로, Slack 및 Discord와 같은 현대 협업/메신저 플랫폼의 기초를 확립했습니다.

**Project Goal**: **ft_irc**는 **RFC 1459** 표준 규격을 준수하여 **C++98** 표준으로 처음부터(Scratch) 직접 개발한 IRC 서버입니다. 본 프로젝트의 핵심 목표는 단일 스레드 환경에서 `poll()` I/O 멀티플렉싱을 활용하여 다중 클라이언트 동시 접속, 채널 관리 및 실시간 메시지 브로드캐스팅을 안전하게 처리하는 고성능 비동기 논블로킹(Non-blocking) 네트워크 서버를 구현하는 것입니다.

### Architecture & Directory Structure

#### 1. Layered Architecture

`ft_irc`는 명확한 관심사 분리(Separation of Concerns)를 위해 크게 3개의 핵심 계층으로 모듈화되어 있습니다:

1. **네트워크 계층 (`Server`, `PollManager`, `Socket`, `Client`)**:
   - 논블로킹 TCP 소켓의 생명주기 관리, `poll()` 기반 I/O 멀티플렉싱, 수신 스트림 버퍼 누적 및 CRLF(`\r\n`) 기준 메시지 경계 분리, 부분 전송(Partial Send)을 안전하게 처리하기 위한 송신 버퍼 큐 관리.
2. **파서 계층 (`Parser`, `Message`)**:
   - 메시지 Prefix, Command, Parameters 및 Trailing 요소의 유효성 검증.
   - 클라이언트 등록 상태 전이(`PASS` ➔ `NICK` ➔ `USER`) 강제 및 해당 명령어 핸들러로의 디스패칭.
3. **채널 및 커맨드 계층 (`Channel`, `ICommand` handlers)**:
   - 채널 상태, 멤버십, 오퍼레이터 권한, 채널 모드 관리 및 RFC 표준 번호 응답(Numeric Reply) 생성.

#### 2. Directory Structure

```text
.
├── Makefile
├── include/
│   ├── client/
│   │   └── Client.hpp           ← 클라이언트 상태 및 송수신 버퍼 관리
│   ├── server/
│   │   ├── Server.hpp           ← 서버 메인 컨트롤러
│   │   ├── PollManager.hpp      ← pollfd 배열 및 I/O 폴링 관리자
│   │   └── Socket.hpp           ← 소켓 설정 및 논블로킹 래퍼
│   ├── parser/
│   │   ├── Parser.hpp           ← 메시지 파서 및 명령어 디스패처
│   │   ├── Message.hpp          ← 파싱된 IRC 메시지 구조체
│   │   └── commands/            ← Pass, Nick, User, Ping, Pong, Quit
│   ├── channel/
│   │   ├── Channel.hpp          ← 채널 상태, 모드 및 멤버 관리
│   │   └── commands/            ← Join, Part, Topic, Mode, Kick, Invite
│   └── common/
│       ├── ICommand.hpp         ← 명령어 인터페이스
│       ├── Replies.hpp          ← RFC 숫자 응답 상수
│       └── Utils.hpp            ← 유틸리티 헬퍼 함수
└── src/
    ├── main.cpp                 ← 프로그램 진입점(Entry point)
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

| 카테고리 | 명령어 | 설명 |
|---|---|---|
| **연결 및 인증** | `PASS` | 서버 비밀번호를 통한 연결 인증 |
| | `NICK` | 클라이언트 닉네임 설정 및 변경 (고유성 검증) |
| | `USER` | 사용자명 및 실명 등록으로 최종 등록 완료 |
| | `PING` / `PONG` | 연결 생존 확인을 위한 하트비트 메커니즘 |
| | `QUIT` | 정상 연결 종료 및 공유 채널에 퇴장 메시지 브로드캐스트 |
| **채널 작업** | `JOIN` | 채널 생성 또는 입장 (비밀번호, 인원제한, 초대 상태 검증) |
| | `PART` | 지정한 채널에서 퇴장 |
| | `TOPIC` | 채널 주제(Topic) 조회 및 변경 |
| | `MODE` | 채널 모드 및 오퍼레이터(운영자) 권한 설정/조회 |
| | `KICK` | 채널에서 특정 사용자 강제 퇴장 (오퍼레이터 전용) |
| | `INVITE` | 초대 전용 채널에 사용자 초대 (오퍼레이터 전용) |
| **메시징** | `PRIVMSG` | 특정 사용자 1:1 메시지 전송 또는 채널 전체 브로드캐스트 |

#### 2. Supported Channel Modes

- `+i` / `-i`: 초대 전용(Invite-only) 채널 설정 / 해제
- `+t` / `-t`: 채널 운영자(Operator)만 `TOPIC`을 변경할 수 있도록 제한 / 해제
- `+k` / `-k`: 채널 비밀번호(Key) 설정 / 해제
- `+o` / `-o`: 채널 운영자(Operator) 권한 부여 / 박탈
- `+l` / `-l`: 채널 최대 접속 인원수(Limit) 제한 / 해제

---

## Instructions

### 1. Build

제공된 `Makefile`을 통해 컴파일합니다 (`-Wall -Wextra -Werror -std=c++98`):

```bash
make            # ircserv 실행 파일 빌드
make clean      # 오브젝트 및 의존성 파일 삭제
make fclean     # 실행 파일 및 모든 빌드 산출물 삭제
make re         # 완전 재빌드
```

### 2. Run

수신 대기할 포트 번호(`1` - `65535`)와 서버 비밀번호를 지정하여 실행합니다:

```bash
./ircserv <port> <password>

# 예시: 6667 포트, 비밀번호 '1234'로 실행
./ircserv 6667 1234
```

### 3. Connect & Test

- **Standard IRC Client (Irssi)**:
  ```bash
  # 한 줄 명령어로 즉시 접속 (추천)
  irssi -c 127.0.0.1 -p 6667 -w 1234 -n mynick
  ```
  또는 Irssi 실행 후 내부 쉘에서 접속:
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
  접속 후 RFC 1459 규격에 맞춰 등록 핸드셰이크 명령어를 직접 입력합니다:
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

- [RFC 1459: Internet Relay Chat Protocol](https://datatracker.ietf.org/doc/html/rfc1459) — IRC 아키텍처, 메시지 형식 및 핵심 명령어 프로토콜 표준 규격.
- [RFC 2812: Internet Relay Chat: Client Protocol](https://datatracker.ietf.org/doc/html/rfc2812) — 최신 클라이언트 상호작용 및 채널 관리를 위한 업데이트된 프로토콜 규격.

### Use of AI

42 커리큘럼 규정을 준수하여 다음과 같은 작업에 투명하게 AI 도구를 활용했습니다:

- **CodeRabbit**:
  - **Code Review**: 잠재적 버그 탐지 및 코드 스타일 일관성 유지를 위한 자동화된 Pull Request(PR) 검토.
- **AI Assistants**:
  - **RFC Clarification**: RFC 1459 표준 규격의 메시지 포맷, 엣지 케이스 및 숫자 응답(Numeric Reply) 코드 확인.
  - **Test Script Assistance**: 회귀 테스트를 위한 파이썬 자동화 테스트 스크립트 작성 보조.
  - **Refactoring Advice**: C++98 표준 문법 확인 및 코드 구조 정리 조언.
