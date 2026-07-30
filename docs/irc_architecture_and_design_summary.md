# IRC 프로젝트 파싱 계층 및 객체지향 아키텍처 정리 문서

---

## 1. C++ 핵심 문법 및 패턴

### 1.1 전방 선언 (Forward Declaration)
* **개념**: `class Server;`, `class Client;`, `class ICommand;` 처럼 클래스의 이름만 컴파일러에게 미리 알려주는 선언 방식.
* **사용 목적**:
  1. **헤더 포함 최적화**: 포인터(`*`)나 참조자(`&`)만 사용할 때는 내부 구조를 몰라도 주소 크기(8바이트)만 다루므로 `#include` 없이 전방 선언만으로 컴파일 가능.
  2. **순환 참조(Circular Include) 방지**: `Server.hpp`와 `Parser.hpp`가 서로를 무한 포함하는 현상 방지.
  3. **컴파일 속도 향상**: 헤더 변경 시 재컴파일 범위 최소화.

### 1.2 C++98 방식의 객체 복사 금지 (Non-copyable 패턴)
* **코드 예시**:
  ```cpp
  private:
      Parser(const Parser&);            // 복사 생성자
      Parser& operator=(const Parser&); // 복사 대입 연산자
  ```
* **이유**: `Parser` 내부에서 동적 할당된 포인터(`ICommand*`)를 소유하므로, 얕은 복사 시 발생하는 **이중 해제(Double Free)** 에러 방지.
* **원리**: `private` 영역에 선언하여 외부 접근을 차단하고, 구현부(body)를 만들지 않아 내부/friend 복사 시 링크 에러 발생시킴.

---

## 2. 파싱 계층 클래스 역할 및 생명주기 (Lifecycle)

| 클래스 | 역할 구분 | 라이프사이클 (Lifecycle) | 주요 특징 |
| :--- | :--- | :--- | :--- |
| **`Parser`** | **서비스 / 디스패처 기계** | 서버 시작 시 **1개 생성**되어 영구 재사용 | `m_commands` 맵에 커맨드 핸들러들을 보관하고, 들어온 메시지에 맞춰 적절한 `ICommand`를 찾아 실행 |
| **`Message`** | **데이터 / 값 객체 (DTO)** | 문자열 한 줄마다 **스택에 순간 생성 후 소멸** | `rawLine`을 `Prefix`, `Command`, `Params`, `Trailing` 토큰으로 분해하여 담는 일회성 가방 |
| **`ICommand`** | **비즈니스 로직 일꾼** | `Parser` 생성 시 동적할당(`new`)되어 맵에 등록 | 다형성(`virtual execute()`)을 통해 명령어별 개별 로직 수행 |

---

## 3. IRC 프로토콜 파싱 및 구분자 (Delimiter)

### 3.1 `Message::parse()` 5단계 분해 흐름
1. **양쪽 공백 탐색**: `start`와 `end` 인덱스 계산 (실제 자르지 않고 인덱스로만 탐색하여 성능 최적화).
2. **Prefix 파싱**: 첫 글자가 `:` 인 경우, 첫 공백(`space`) 전까지 잘라내어 `Prefix` 저장.
3. **Trailing 마커 탐색**: `" :"` (공백+콜론) 위치 탐색 ➡️ 마커 기준 왼쪽(`middle`)과 오른쪽(`trailing`) 구분.
4. **Middle 구간 토큰화**: 공백(`' '`) 기준으로 split ➡️ 0번째는 `Command`, 1번째부터는 `Params` 저장.
5. **Trailing 저장**: `" :"` 오른쪽 문장을 공백 보존한 채 통째로 `Trailing`에 저장.

### 3.2 구분자 체계
* **1차 구분자 (Message::parse 단계)**
  * **행(줄) 구분자**: `\r\n` (CRLF) ➡️ Network 계층에서 버퍼 누적 후 잘라서 전달.
  * **토큰 구분자**: 공백(`' '`, Space) ➡️ Prefix, Command, Params 사이를 자르는 유일한 기준.
* **2차 구분자 (Command 실행 단계 - Phase 2)**
  * **쉼표(`,`)**: `JOIN #chan1,#chan2` 또는 `PRIVMSG user1,user2` 처럼 파라미터 내부에서 다중 대상을 분리할 때 사용 (각 커맨드 로직 내부에서 `Utils::split` 활용).

---

## 4. 커맨드 확장과 다형성 (Open-Closed Principle)

새로운 커맨드(예: `JOIN`, `PART`, `MODE` 등)가 추가될 때:
* **`Parser` 생성자**: `registerCommand("JOIN", new Join());` 등록 코드 **1줄 추가**.
* **`Parser` 멤버 함수**: **단 1개도 새로 만들 필요 없음!**
* **이유**: 모든 커맨드는 `ICommand` 인터페이스의 `virtual void execute(Server&, Client&, const Message&)` 공통 메서드를 구현하므로 `Parser`는 `it->second->execute(server, client, msg);` 한 줄로 모든 커맨드를 통일되게 호출함.

---

## 5. Server & Client 필수 특수 헬퍼 함수 (명령어에 의한 요구사항)

단순한 Getter/Setter를 넘어서, 각 명령어(Command)의 비즈니스 로직을 수행하기 위해 `Server`와 `Client`에 반드시 정의되어야 하는 핵심 헬퍼 함수들입니다.

### 5.1 `Server` 클래스 필수 헬퍼 함수
1. **닉네임 관리 (NICK, QUIT, PRIVMSG 관련)**
   * `bool isNicknameInUse(const std::string& nick)`: 닉네임 중복 체크 (`ERR_NICKNAMEINUSE` 응답용).
   * `void registerNickname(const std::string& nick, Client& client)`: 닉네임 등록 및 갱신.
   * `void releaseNickname(const std::string& nick)`: 닉네임 변경 시 이전 닉네임 해제 또는 유저 `QUIT` 시 닉네임 반납.
   * `Client* getClientByNickname(const std::string& nick)`: 1:1 대화(`PRIVMSG`)나 초대(`INVITE`) 시 상대방 `Client` 인스턴스 검색.
2. **채널 관리 (JOIN, PART, PRIVMSG 관련)**
   * `Channel* getChannel(const std::string& name)`: 특정 채널 검색.
   * `Channel& getOrCreateChannel(const std::string& name)`: `JOIN` 시 채널이 없으면 새로 생성해서 반환.
   * `void removeChannelIfEmpty(const std::string& name)`: `PART`나 `QUIT`으로 채널 인원이 0명이 되면 채널 삭제 및 자원 해제.
3. **연결 종료 처리 (QUIT 관련)**
   * `void disconnectClient(Client& client, const std::string& quitReason)`: 클라이언트 접속 끊김 시 그 유저가 들어가 있던 모든 채널에서 퇴장 처리(`PART` 브로드캐스트) 및 자원 정리.

### 5.2 `Client` 클래스 필수 헬퍼 함수
1. **등록(인증) 완성 시퀀스 (PASS ➡️ NICK ➡️ USER 완료 처리)**
   * `bool isRegistered() const`: 등록 완료 여부 플래그.
   * `void checkAndCompleteRegistration()`: PASS/NICK/USER 3가지가 모두 제대로 채워진 바로 그 순간, 클라이언트에게 웰컴 메시지(`RPL_WELCOME 001~004`)를 한 번만 쏘아주고 `isRegistered = true`로 변경해 주는 헬퍼 함수.
2. **응답 메시지 큐잉 (Network 전송 레이어 경계)**
   * `void queueReply(const std::string& line)`: 단순 속성 작성이 아니라, 전송할 응답 텍스트를 `m_outbox` 버퍼에 누적(`+=`)해 두고, 소켓이 쓰기 가능(`writable`)해질 때 전송되도록 예약하는 유일한 응답 통로 함수.
3. **유저가 참여 중인 채널 목록 관리 (QUIT 메시지 전파용)**
   * `void joinChannel(Channel* channel)` / `void leaveChannel(Channel* channel)`
   * `const std::vector<Channel*>& getJoinedChannels() const`: 특정 유저가 `QUIT`으로 갑자기 나갔을 때, 그 유저가 참여 중이던 모든 채널의 다른 멤버들에게 `"User has quit"` 메시지를 쏘아주기 위해 유저 스스로가 자기가 속한 채널 목록을 들고 있어야 함.

### 5.3 요약 및 협업 요구사항 제안 표

| 클래스 | 추천 헬퍼 함수 | 관련 명령어 |
| :--- | :--- | :--- |
| **Server** | `isNicknameInUse()`, `releaseNickname()` | `NICK`, `QUIT` |
| **Server** | `getClientByNickname()` | `PRIVMSG`, `INVITE` |
| **Server** | `getOrCreateChannel()`, `removeChannelIfEmpty()` | `JOIN`, `PART` |
| **Client** | `checkAndCompleteRegistration()` | `PASS`, `NICK`, `USER` |
| **Client** | `queueReply()` | 전체 명령어 응답 |
| **Client** | `getJoinedChannels()` | `QUIT` |

---

## 6. 팀 3인 역할 분담 및 객체지향 아키텍처

`Server`와 `Client` 클래스는 단 한 사람의 소유가 아니며, **3명의 팀원이 각자 담당 영역에 필요한 헬퍼 함수를 기여하는 공동 작업 구역**입니다.

### 6.1 3인 역할 분담표
* **Network 담당자 (통신 엔진)**:
  * `socket`, `bind`, `listen`, `poll()` 이벤트 루프, non-blocking I/O.
  * `Client::queueReply()`, `Client::getOutbox()` / `clearOutbox()` 구현.
* **Parser 담당자 (파싱 & 등록 시퀀스)**:
  * `Message::parse()`, `Parser::process()`, `PASS`, `NICK`, `USER`, `PING`, `PONG`, `QUIT` 구현.
  * `Client::checkAndCompleteRegistration()`, `Server::isNicknameInUse()` 구현.
* **Channel / Cmd 담당자 (채널 및 모드)**:
  * `Channel` 클래스 전체, `JOIN`, `PART`, `TOPIC`, `MODE`, `KICK`, `INVITE` 구현.
  * `Server::getOrCreateChannel()`, `Server::removeChannelIfEmpty()`, `Client::joinChannel()` 구현.

---

## 7. 닉네임 관리와 위임(Delegation) 패턴 및 적정 설계 (Pragmatic Choice)

닉네임 목록 및 중복 체크 로직을 `NicknameRegistry` (또는 `NickHandler`)로 분리할 때의 아키텍처와 실무적 선택 기준:

### 7.1 포함 및 위임 구조 (Has-A 관계)
* **`NicknameRegistry` (독립 일꾼)**: 닉네임 맵(`std::map<std::string, Client*>`)과 순수 CRUD 기능 보유. `Server`의 존재를 모름.
* **`Server` (주인)**: `NicknameRegistry`를 내부 멤버 변수(`m_nickRegistry`)로 **소유(포함)**하고, 외부에 기존 간판 메서드만 유지한 채 실제 일은 내부 일꾼에게 위임(Delegate).

```text
[External Command] ──> server.isNicknameInUse("bob")
                             │
                             ▼ (위임)
                       m_nickRegistry.isInUse("bob")
```

```cpp
// Server.hpp 예시
#include "NicknameRegistry.hpp"

class Server
{
public:
    bool isNicknameInUse(const std::string& nick) const {
        return m_nickRegistry.isInUse(nick); // 내부 일꾼에게 위임!
    }

private:
    NicknameRegistry m_nickRegistry; // 소유
};
```

### 7.2 실무적 아키텍처 비교 & 적정 설계 선택 (Pragmatic Choice)

각 기능(닉네임, 채널, 연결해제, 등록 등)마다 별도의 위임 핸들러 클래스를 만들 것인가에 대한 실무 비교:

* ❌ **방식 A: 6개의 개별 위임 핸들러 클래스로 모두 분리하는 방식 (오버 엔지니어링)**
  * **문제점**: 작은 프로젝트 규모에 비해 클래스 파일(12개+)이 폭발하여 지저분해짐. 클래스 간 포인터 전달(의존성)이 꼬여 복잡도가 대폭 증가함.
* ✅ **방식 B: `Server`와 `Client` 내부에 헬퍼 메서드로 직접 만드는 방식 (실무 강력 권장 ★★★)**
  * **장점**: **KISS 원칙 (Keep It Simple, Stupid)** 준수. 추가 클래스 0개로 `Server`와 `Client` 기존 구조 유지. `server.isNicknameInUse()`, `client.queueReply()` 처럼 직관적인 가독성과 빠른 개발/디버깅 속도 확보.

> **최종 결론**: 닉네임 로직이 수천 줄에 달하고 외부 DB 통신이 필요한 대규모 시스템이 아닌 **현재 ft_irc 프로젝트 수준에서는 `Server` / `Client` 내 헬퍼 메서드로 직접 구현하는 방식 B가 가장 적절하고 우수한 설계**입니다.
