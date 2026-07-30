# [특강] C++ IRC 서버의 계층형 아키텍처(Layered Architecture) 및 모듈 연결 가이드

> **강의 목표**: C++ IRC 서버 프로젝트의 전체 구조를 이해하고, 네트워크 계층(`Server`)과 파서 계층(`Parser`), 도메인 계층(`Client`, `Channel`) 간의 역할 분담 및 연결 방법을 단단히 확립한다.

---

## 1. 서론: 왜 아키텍처(Architecture)를 고민해야 하는가?

반갑습니다, 학생 여러분. C++로 네트워크 서버를 만들다 보면 초반에 가장 흔히 겪는 시행착오가 있습니다. 
바로 **"소켓 읽는 코드 안에 파싱 로직이 들어가고, 파싱 로직 안에 채널 입장/퇴장 로직이 다 뒤엉켜 버리는 문제(Spaghetti Code)"**입니다.

이렇게 작성된 코드는 한 곳을 수정하면 예측할 수 없는 다른 곳에서 에러가 터지고, 팀원들과 업무를 나누어 개발하기가 매우 어려워집니다.

이를 해결하기 위해 우리는 **소프트웨어의 역할을 명확히 나누는 "계층형 아키텍처(Layered Architecture)"**를 도입합니다.

---

## 2. 3단계 계층 구조 (Layered Architecture)

IRC 서버는 크게 **3개의 레이어**로 구분할 수 있습니다.

```text
┌─────────────────────────────────────────────────────────┐
│  Layer 1: Network / Infrastructure Layer (최상단 계층)   │
│  - 주요 객체: Server, PollManager, Socket                │
│  - 핵심 역할: 소켓 I/O 이벤트 감지 및 Raw 문자열 추출      │
└────────────────────────────┬────────────────────────────┘
                             │ 호출 (Message Line)
                             ▼
┌─────────────────────────────────────────────────────────┐
│  Layer 2: Protocol / Parsing Layer (중간 계층)           │
│  - 주요 객체: Parser, Message, ICommand (Pass, Nick 등)  │
│  - 핵심 역할: 프로토콜 문법 해석 및 해당 명령어 라우팅   │
└────────────────────────────┬────────────────────────────┘
                             │ 호출 (Business Logic)
                             ▼
┌─────────────────────────────────────────────────────────┐
│  Layer 3: Domain / Business Logic Layer (최하단 계층)    │
│  - 주요 객체: Client, Channel, Server State              │
│  - 핵심 역할: IRC 규칙 검증, 닉네임/채널 상태 관리      │
└────────────────────────────┴────────────────────────────┘
```

---

### [Layer 1] Network / Infrastructure Layer (최상단 계층)
* **담당 클래스**: `Server`, `PollManager`, `Socket`
* **주요 책임**: 
  * OS 레벨의 non-blocking 소켓 생성 및 `poll()` 이벤트 모니터링
  * 클라이언트 접속/해제 처리
  * 바이트 스트림을 수신하여 개행(`\r\n`) 기준의 완벽한 문장(`line`) 추출
* **관심사**: **"언제 데이터가 들어왔는가? 어떻게 비동기로 읽고 보낼 것인가?"**
* 💡 *네트워크 레이어는 읽어들인 문자열이 `NICK`인지 `JOIN`인지 전혀 몰라도 됩니다.*

---

### [Layer 2] Protocol / Parsing Layer (중간 계층)
* **담당 클래스**: `Parser`, `Message`, `ICommand` 구현체들 (`Pass`, `Nick`, `User` 등)
* **주요 책임**:
  * raw 문자열을 RFC 1459 규격에 맞춰 prefix, command, params로 분해
  * 명령어 상표(Command Name)를 보고 적절한 명령어 핸들러 객체에 실행 위임
* **관심사**: **"이 문장은 정상적인 IRC 규격인가? 어떤 핸들러에게 일을 시켜야 하는가?"**

---

### [Layer 3] Domain / Business Logic Layer (최하단 계층)
* **담당 클래스**: `Client`, `Channel`, `Server` 내 데이터 저장소
* **주요 책임**:
  * `Client`: 소켓 fd, 닉네임, 인증(PASS) 상태, 입력/출력 버퍼 관리
  * `Channel`: 채널 이름, 참여한 클라이언트 목록, 방장 권한, 토픽, 채널 모드 관리
  * 실제 비즈니스 규칙(예: "중복 닉네임인가?", "채널 비밀번호가 맞나?") 검증
* **관심사**: **"현재 서버와 채널, 클라이언트의 상태를 어떻게 올바르게 유지할 것인가?"**

---

## 3. 핵심 아키텍처 원칙: 단방향 의존성 (Unidirectional Dependency)

> **"상위 계층은 하위 계층을 알아도 되지만, 하위 계층은 상위 계층을 절대 몰라야 한다."**

이것이 계층형 아키텍처의 황금 규칙입니다.

1. **상위 ➔ 하위 참조 허용**:
   * `Server`(Layer 1)는 `Parser`(Layer 2)를 소유하고 호출합니다.
   * `Parser`(Layer 2)는 `Client`/`Channel`(Layer 3)의 상태를 변경하도록 일을 시킵니다.
2. **하위 ➔ 상위 참조 금지**:
   * `Channel`(Layer 3)은 자신을 조작하는 존재가 `Parser`인지, `Server`인지, 혹은 테스트 코드인지 **알 필요가 없습니다.**
   * `Channel`은 오직 자신에게 들어온 `Client` 객체의 목록을 추가/제거하는 자기 자신의 상태 관리 로직에만 집중합니다.

---

## 4. 실전 연결 Guide: `Server::handleLine`에서 `Parser` 호출하기

네트워크 계층에서 파서 계층으로 데이터가 넘어가는 핵심 접점은 `Server::handleLine` 메서드입니다.

### [연결 구조 설계]
1. `Server` 클래스가 `Parser m_parser;` 객체를 멤버 변수로 소유합니다.
2. 네트워크 계층이 개행(`\r\n`) 단위로 분리된 문장(`line`)을 완성하면 `handleLine()`을 호출합니다.
3. `handleLine()`은 자신의 참조(`*this`), 클라이언트 참조(`*client`), 그리고 문장(`line`)을 `Parser::process()`에 넘깁니다.

### [호출 흐름 요약]
```text
Server::run()
  └── Server::receiveFromClient(fd)
        └── Client::extractLine(line) 성공!
              └── Server::handleLine(client, line)
                    └── Parser::process(*this, *client, line)
                          └── Command::execute(*this, *client, msg)
```

---

## 5. C++ 실무 팁 (개발 시 주의사항)

### 팁 1: `*this`와 `*client` (포인터 vs 참조)
* `handleLine(Client *client, ...)`의 `client`는 **포인터(`Client*`)**입니다.
* `Parser::process(Server& server, Client& client, ...)`는 **참조(`Client&`)**를 요구합니다.
* C++에서 포인터를 참조로 넘길 때는 주소 해제 연산자 `*`를 붙여 **`*client`** 형태로 넘깁니다.
* 자기 자신(`Server`) 인스턴스를 참조로 넘길 때는 **`*this`**를 넘깁니다.

### 팁 2: 헤더 순환 참조 (Circular Include) 방지 ⚠️
C++ 개발 시 `Server.hpp`와 `Parser.hpp`가 서로를 `#include`하면 컴파일 오류가 발생합니다.

* **해결책 (전방 선언, Forward Declaration)**:
  헤더 파일(`*.hpp`)에는 `#include` 대신 클래스의 존재만 알려주는 `class Server;`, `class Client;` 전방 선언을 사용하고, 실제 `#include`는 소스 파일(`*.cpp`) 내부에서만 작성합니다.

```cpp
// Parser.hpp 예시
#ifndef PARSER_HPP
#define PARSER_HPP

#include <string>

// 전방 선언 (Forward Declaration)
class Server;
class Client;

class Parser {
public:
    void process(Server& server, Client& client, const std::string& rawLine);
};

#endif
```

### 팁 3: 비동기 I/O와 응답 버퍼(OutBuffer) 연동
* `Parser` 내의 명령어(`Command::execute`)가 실행되면 클라이언트에게 보낼 응답 메시지가 `client.queueReply(...)` 또는 `client.appendToOutBuffer(...)`를 통해 클라이언트 개인의 출력 버퍼에 채워집니다.
* `handleLine()`이 끝나는 시점에 해당 클라이언트의 출력 버퍼가 비어있지 않다면, `PollManager`를 통해 소켓의 쓰기 이벤트(`POLLOUT`)를 활성화해야 비동기로 응답 전송이 이루어집니다.

---

## 6. 강의 요약 및 결론

1. **역할 분리**: `Server`는 네트워크 I/O만, `Parser`는 프로토콜 해석만, `Channel`/`Client`는 상태 관리만 담당합니다.
2. **단방향 의존성**: 상위 레이어가 하위 레이어를 호출하며, 하위 레이어는 상위 레이어의 존재를 몰라도 독립적으로 동작해야 합니다.
3. **깔끔한 C++ 코드**: 전방 선언(Forward Declaration)과 적절한 포인터/참조 활용으로 헤더 의존성을 최소화합시다.

이 원칙을 지켜 구현하면 확장성 높고 유지보수가 쉬운 훌륭한 C++ IRC 서버가 만들어집니다. 
질문이 있다면 언제든 편하게 주저하지 말고 물어보세요!
