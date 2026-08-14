# ft_irc 파서 및 기본 명령어 테스트
---

---

## 🔑 1. PASS (서버 비밀번호 인증)

서버 접속 후 가장 먼저 클라이언트가 전송해야 하는 인증 명령어입니다.

### 1-1. 명령어 형식
```text
PASS <password>
```

### 1-2. 테스트 케이스 및 출력 결과

```bash
# [정상 케이스] 올바른 비밀번호 전송
PASS 1234
# 정상 응답: (서버 응답 없음 / Silent)
# 설명: 올바른 비밀번호인 경우 서버는 응답을 보내지 않고 내부 인증 플래그(hasCorrectPassword)만 true로 설정합니다.
# (단, NICK과 USER가 이미 완료된 상태에서 PASS를 마지막으로 보낸 경우 등록이 완료되어 001 RPL_WELCOME 전송)
# -> :ircserv 001 alice :Welcome to the IRC network, alice

# [에러 1] 비밀번호 인자 누락 (461 ERR_NEEDMOREPARAMS)
PASS
# 에러 응답: :ircserv 461 * PASS :Not enough parameters

# [에러 2] 비밀번호 불일치 (464 ERR_PASSWDMISMATCH)
PASS wrongpass
# 에러 응답: :ircserv 464 * :Password incorrect
# 💡 핵심: 비밀번호가 틀려도 소켓 연결을 즉시 끊지 않고 세션을 유지하여, 올바른 PASS를 다시 전송할 수 있는 기회를 제공합니다.

# [에러 3] 이미 등록 완료된 상태에서 PASS 재전송 (462 ERR_ALREADYREGISTRED)
PASS 1234
# 에러 응답: :ircserv 462 alice :You may not reregister
```

---

## 2. NICK (닉네임 설정 및 변경)

클라이언트의 닉네임을 설정하거나 이미 등록된 유저가 닉네임을 변경할 때 사용합니다.

### 2-1. 명령어 형식
```text
NICK <nickname>
```

### 2-2. 닉네임 유효성 규칙 (RFC 1459 + Irssi 호환)
* **길이**: 1자 이상 ~ 최대 9자
* **첫 글자**: 반드시 영문 알파벳 (`[a-zA-Z]`)으로 시작
* **이후 글자**: 영문 알파벳, 숫자 (`0-9`), 허용 특수문자 (`-`, `[`, `]`, `\`, `` ` ``, `^`, `{`, `}`, `_`, `|`)
* **대소문자 동등성 (RFC 1459 2.2)**: `[`와 `{`, `]`와 `}`, `\`와 `|`는 동일 문자로 취급

### 2-3. 테스트 케이스 및 출력 결과

```bash
# [정상 케이스 1] 미등록 상태에서 유효한 닉네임 설정
NICK alice
# 정상 응답: (서버 응답 없음 / Silent)
# 설명: 등록 진행 중에는 닉네임만 세팅되고 별도 응답 없음 (PASS/USER 완료 시 001 환영 메시지 수신)

# [정상 케이스 2] 이미 사용 중인 본인의 닉네임과 동일한 닉네임 재전송
NICK alice
# 정상 응답: (서버 응답 없음 / 무시)
# 설명: 본인의 현재 닉네임과 동일한 닉네임 전송 시 에러(433) 없이 no-op 처리됩니다.

# [정상 케이스 3] 등록 완료 후 닉네임 변경 (클라이언트 2명이 채널에 함께 있을 때)
NICK alice_new
# 브로드캐스트 응답: :alice!alice@127.0.0.1 NICK :alice_new
# 설명: 본인 및 공유 채널 멤버들에게 닉네임 변경 알림이 브로드캐스트됩니다.

# [에러 1] 닉네임 파라미터 누락 (431 ERR_NONICKNAMEGIVEN)
NICK
# 에러 응답: :ircserv 431 * :No nickname given

# [에러 2] 숫자로 시작하는 잘못된 닉네임 (432 ERR_ERRONEUSNICKNAME)
NICK 123alice
# 에러 응답: :ircserv 432 * 123alice :Erroneous nickname

# [에러 3] 특수문자로 시작하는 닉네임 (432 ERR_ERRONEUSNICKNAME)
NICK [badnick
# 에러 응답: :ircserv 432 * [badnick :Erroneous nickname

# [에러 4] 9자를 초과하는 닉네임 (432 ERR_ERRONEUSNICKNAME)
NICK toolongnickname
# 에러 응답: :ircserv 432 * toolongnickname :Erroneous nickname

# [에러 5] 다른 클라이언트가 이미 사용 중인 닉네임 (433 ERR_NICKNAMEINUSE)
# (다른 터미널에서 bob이 접속 중인 상태)
NICK bob
# 에러 응답: :ircserv 433 * bob :Nickname is already in use
```

---

## 3. USER (사용자 정보 등록)

클라이언트의 사용자명(username), 호스트명, 서버명, 실명(realname)을 지정합니다.

### 3-1. 명령어 형식
```text
USER <username> <hostname> <servername> :<realname>
```

### 3-2. 파라미터 상세 설명
* `<username>`: 유저 식별자 (접속 후 변경 불가)
* `<hostname>`, `<servername>` (예: `0 *`): RFC 1459 규격상 위치를 맞추기 위해 존재하지만, 보안 및 단일 서버 아키텍처 특성상 서버가 소켓 IP를 직접 조회하므로 `0`, `*` 등 임의의 값을 넣어도 무방합니다. (irssi 호환을 위해 문법을 준수합니다.)
* `:<realname>`: 유저의 실명. 콜론(`:`)으로 시작하여 공백을 포함하는 Trailing 문자열로 처리됩니다.

### 3-3. 테스트 케이스 및 출력 결과

```bash
# [정상 케이스] 올바른 4개 파라미터 전송
USER alice 0 * :Alice Wonderland
# 정상 응답: (서버 응답 없음 / Silent)
# (단, PASS와 NICK이 이미 완료된 상태에서 USER를 마지막으로 보낸 경우 최종 001 RPL_WELCOME 전송)
# -> :ircserv 001 alice :Welcome to the IRC network, alice

# [에러 1] 파라미터 개수 부족 (4개 미만) (461 ERR_NEEDMOREPARAMS)
USER alice
# 에러 응답: :ircserv 461 alice USER :Not enough parameters

USER alice 0 *
# 에러 응답: :ircserv 461 alice USER :Not enough parameters

# [에러 2] 이미 등록 완료된 상태에서 USER 재전송 (462 ERR_ALREADYREGISTRED)
USER alice 0 * :Alice Wonderland
# 에러 응답: :ircserv 462 alice :You may not reregister
# 설명: USER 정보는 클라이언트의 고유 신원이므로 등록 완료 후에는 재등록/수정이 불가합니다.
```

---

## 4. 등록 시퀀스 (Client Registration)

IRC 서버는 **PASS(인증) + NICK(닉네임) + USER(유저명)** 3가지가 모두 성공적으로 접수되어야 클라이언트를 정식 등록 유저로 승인하고 `001 RPL_WELCOME`을 반환합니다.

### 5-1. 등록 순서의 유연성 (총 6가지 조합 지원)
클라이언트는 어떤 순서로 3개 명령어를 보내도 정상 등록됩니다:

| 번호 | 명령어 전송 순서 | 001 환영 메시지 발생 시점 |
| :---: | :--- | :--- |
| **1** | `PASS` ➔ `NICK` ➔ `USER` | `USER` 전송 완료 시 |
| **2** | `PASS` ➔ `USER` ➔ `NICK` | `NICK` 전송 완료 시 |
| **3** | `NICK` ➔ `PASS` ➔ `USER` | `USER` 전송 완료 시 |
| **4** | `NICK` ➔ `USER` ➔ `PASS` | `PASS` 전송 완료 시 |
| **5** | `USER` ➔ `PASS` ➔ `NICK` | `NICK` 전송 완료 시 |
| **6** | `USER` ➔ `NICK` ➔ `PASS` | `PASS` 전송 완료 시 |

#### 등록 완료 시 서버 응답:
```text
:ircserv 001 alice :Welcome to the IRC network, alice
```
* `:ircserv`: 서버의 고유 식별자 (상수 정의)
* `001`: `RPL_WELCOME` 숫자 코드
* `alice`: 등록된 클라이언트의 닉네임

---

### 4-2. 미등록 상태에서의 명령어 제약 (451 ERR_NOTREGISTERED)

등록을 마치지 않은 클라이언트는 사전에 허용된 명령어(`PASS`, `NICK`, `USER`, `QUIT`, `PING`, `PONG`, `CAP`) 외의 명령어를 실행할 수 없습니다.

```bash
# 미등록 상태에서 JOIN 시도
JOIN #42
# 에러 응답: :ircserv 451 * :You have not registered

# 미등록 상태에서 PRIVMSG 시도
PRIVMSG bob :hello
# 에러 응답: :ircserv 451 * :You have not registered

# 등록 완료 후 존재하지 않는 알 수 없는 명령어 전송
FOOBAR arg
# 에러 응답: :ircserv 421 alice FOOBAR :Unknown command
```

---

## 5. PING (서버 생존 확인 요청)

클라이언트가 서버의 생존 상태와 지연 시간을 확인하기 위해 전송합니다.

### 5-1. 명령어 형식
```text
PING <token>
PING :<token>
PING <token> <target_server>
```

### 5-2. 테스트 케이스 및 출력 결과

```bash
# [정상 케이스 1] 서버 이름을 토큰으로 전달
PING ircserv
# 정상 응답: :ircserv PONG ircserv :ircserv

# [정상 케이스 2] 임의의 문자열 토큰 전달 (middle 파라미터)
PING hello123
# 정상 응답: :ircserv PONG ircserv :hello123

# [정상 케이스 3] 콜론을 포함한 Trailing 토큰 전달
PING :token456
# 정상 응답: :ircserv PONG ircserv :token456

# [정상 케이스 4] 2개 인자 전달 (token + target_server)
# target_server가 본 서버('ircserv', 대소문자 무관)인 경우 정상 응답
PING mytoken ircserv
# 정상 응답: :ircserv PONG ircserv :mytoken

PING mytoken IRCSERV
# 정상 응답: :ircserv PONG ircserv :mytoken

# [에러 1] 토큰 파라미터 누락 (409 ERR_NOORIGIN)
PING
# 에러 응답: :ircserv 409 alice :No origin specified

# [에러 2] 존재하지 않는 대상 서버 지정 (402 ERR_NOSUCHSERVER)
PING mytoken otherserver
# 에러 응답: :ircserv 402 alice otherserver :No such server
```

---

## 6. PONG (클라이언트 응답)

서버가 클라이언트에게 PING을 보냈을 때 클라이언트가 답장하는 메시지입니다.

### 6-1. 명령어 형식
```tex
PONG <servername>
PONG :<servername>
```

### 6-2. 테스트 케이스 및 출력 결과

```bash
# [정상 케이스 1]
PONG ircserv
# 정상 응답: (서버 응답 없음 / No reply)

# [정상 케이스 2]
PONG :ircserv
# 정상 응답: (서버 응답 없음 / No reply)
```
---

## 7. QUIT (세션 종료)

클라이언트가 서버와의 연결을 정상적으로 종료할 때 사용합니다.

### 7-1. 명령어 형식
```text
QUIT [:<사유>]
```

### 7-2. 테스트 케이스 및 출력 결과

```bash
# [정상 케이스 1] 종료 사유를 포함하여 QUIT
QUIT :Leaving server
# 정상 응답: ERROR :Closing Link: Leaving server
# 설명: 서버는 클라이언트에게 ERROR 메시지를 전송한 직후 소켓 연결(FD)을 정상 종료합니다.

# [정상 케이스 2] 사유 없이 QUIT 전송
QUIT
# 정상 응답: ERROR :Closing Link: Client Quit
# 설명: 사유를 생략한 경우 기본 사유("Client Quit")를 붙여 Closing Link 전송 후 소켓을 종료합니다.
```

> 💡 **채널 브로드캐스트 동작**:
> * 종료하는 유저가 참여 중이던 채널이 있는 경우, 채널에 남아있는 다른 유저들에게 아래와 같은 QUIT 알림이 브로드캐스트됩니다:
>   ```text
>   :alice!alice@127.0.0.1 QUIT :Leaving server
>   ```
> * QUIT 명령어는 어떤 형식으로 들어와도 에러를 발생시키지 않고 항상 세션을 안전하게 정리하고 종료합니다.

---

### 🧪 시나리오 A. 정상 등록 ➔ PING ➔ QUIT (해피 패스)
```text
PASS 1234
NICK alice
USER alice 0 * :Alice Wonderland
PING ircserv
QUIT :Bye bye!
```
**기대 출력:**
```text
:ircserv 001 alice :Welcome to the IRC network, alice
:ircserv PONG ircserv :ircserv
ERROR :Closing Link: Bye bye!
```

---

### 🧪 시나리오 B. 비밀번호/닉네임/유저 에러 발생 및 복구
```text
PASS wrongpass
PASS
NICK 123badnick
NICK
USER incomplete
PASS 1234
NICK bob
USER bob 0 * :Bob The Builder
```
**기대 출력:**
```text
:ircserv 464 * :Password incorrect
:ircserv 461 * PASS :Not enough parameters
:ircserv 432 * 123badnick :Erroneous nickname
:ircserv 431 * :No nickname given
:ircserv 461 * USER :Not enough parameters
:ircserv 001 bob :Welcome to the IRC network, bob
```

---

### 🧪 시나리오 C. 등록 완료 후 중복 전송 방지 및 알 수 없는 명령어
*(시나리오 A 또는 B로 등록 완료 후 입력)*
```text
PASS 1234
USER alice 0 * :Alice Again
NICK alice
UNKNOWNCMD test
```
**기대 출력:**
```text
:ircserv 462 alice :You may not reregister
:ircserv 462 alice :You may not reregister
(NICK alice는 본인 닉네임이므로 아무 응답 없음)
:ircserv 421 alice UNKNOWNCMD :Unknown command
```

---