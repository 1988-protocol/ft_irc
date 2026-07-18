# PLAN

3인 팀 기준 3주(mandatory) 진행 계획. <br>
[README](./README.md)의 Ownership과 동일하게 Network / Parser / Channel/Cmd 사용.

## 핵심 원칙

1. 데이터 흐름은 `raw bytes → 버퍼 누적/분리 (Network) → 파싱 (Parser) → 커맨드 실행 (Channel/Cmd) → 응답 생성 → 전송 (Network)` 순서의 레이어드 구조. 완전 병렬 작업은 불가능하므로 **Phase 0에서 인터페이스를 먼저 확정**한다.
2. poll() 이벤트 루프와 fd 순회는 Network가 단독 소유한다. Parser/Channel은 이벤트 루프를 몰라도 되고, 각각 "파싱된 한 줄(`Message`)"과 "커맨드 실행 요청(`ICommand`)" 인터페이스만 알면 된다.
3. 함수 시그니처 변경은 사전 공지 후 진행한다. 통합 시점 대량 충돌의 가장 흔한 원인.

---

## Phase 0 — 설계 (Day 1~2, 전원 참여)

- [ ] `Server` / `Client` / `Channel` / `ICommand` / `Message` 시그니처 확정 (구현은 비워둠)
- [ ] 레이어 간 인터페이스 정의
  - Network → Parser: 누적 버퍼에서 `\r\n` 단위로 잘라낸 한 줄 (`std::string`)
  - Parser → Channel/Cmd: `Message{ prefix, command, params, trailing }`
  - 각 커맨드 핸들러 → Network: 전송할 응답 문자열
- [ ] numeric reply 헬퍼 시그니처 합의
  - 예: `std::string reply(int code, std::string target, std::string msg)` → `":server 461 nick CMD :msg\r\n"`
- [ ] 커맨드 배분 확정 (아래 표)
- [ ] Git 전략: `main` / `dev` / `feature-*`, PR 필수, 매일 `dev` 머지, 하루 1회 통합 빌드 확인
- [ ] Makefile, norm 규칙(클래스당 파일 분리 등) 합의

### 커맨드 배분

| 담당 | 커맨드 |
|---|---|
| Parser | PASS, NICK, USER, PING, PONG, QUIT |
| Channel/Cmd | JOIN, PART, TOPIC, MODE, KICK, INVITE |
| 공동 | PRIVMSG, NOTICE (유저 대상은 Parser, 채널 대상은 Channel/Cmd와 협의) |

---

## Phase 1 — 개별 기능 뼈대 (Day 3~7)

### Network

- [ ] `socket()`/`bind()`/`listen()`, poll() 이벤트 루프, `accept()`
- [ ] non-blocking 설정 (`fcntl(fd, F_SETFL, O_NONBLOCK)`)
- [ ] client별 recv buffer 누적 + `\r\n` 분리 → Parser 전달 인터페이스 준비
- [ ] SIGPIPE 대응: `sigaction(SIGPIPE, SIG_IGN)` + `send()` EPIPE 체크
- [ ] disconnect 감지(`recv() == 0`) 및 자원 정리(fd close, Client 제거)
- [ ] 테스트: `nc`로 echo 동작 확인

### Parser

- [ ] Message 파싱 (prefix / command / params / trailing)
  - 엣지 케이스: 중복 공백, 빈 파라미터, trailing 없는 경우
- [ ] 커맨드 dispatcher 골격 (`std::map<std::string, HandlerFunc>` 등)
- [ ] PASS/NICK/USER 등록 시퀀스, 미등록 상태에서 타 커맨드 거부
- [ ] 에러 응답 코드 정리 (431, 432, 433, 451, 461 등)
- [ ] 테스트: mock Client로 파싱 단위 테스트

### Channel/Cmd

- [ ] `Channel` 클래스: name, topic, members, operators, invite list, mode(i/t/k/o/l), password, user limit
- [ ] JOIN / PART / PRIVMSG(채널 대상) 기본 로직 — mock Server/Client로 단위 테스트
- [ ] KICK / INVITE / TOPIC / MODE 초안 (권한 체크 우선 설계)
- [ ] 테스트: mock으로 생성/입장/퇴장/모드변경 시나리오

**Day 7 체크포인트:** 3파트 골격 완성 + Phase 0 인터페이스 시그니처 변경 없음 확인.

---

## Phase 2 — 통합 + 심화 (Day 8~14)

### Day 8~9: 1차 통합

- [ ] Network poll 루프에 Parser 연결 → dispatcher까지 실제 흐름 연결
- [ ] Channel 로직을 dispatcher에 연결
- [ ] 실제 클라이언트(irssi/HexChat/WeeChat)로 PASS/NICK/USER 정상 통과 확인
- [ ] Network-Parser, Parser-Channel 접점 페어 디버깅 일정 여유 확보

### Day 10~14: 심화

| 담당 | 작업 |
|---|---|
| Network | 다중 클라이언트 동시 처리, partial send 처리(EAGAIN/EWOULDBLOCK 시 송신 큐잉), QUIT 처리, 서버 종료 시 자원 정리 |
| Parser | 전체 커맨드 파라미터 검증, 커맨드별 numeric reply, PRIVMSG/NOTICE(유저 대상), PING/PONG |
| Channel/Cmd | KICK/INVITE/TOPIC/MODE 전체 구현, 권한 없음/대상 없음 에러 처리, 다중 채널 동시 운영 |

**Day 14 체크포인트:** mandatory 커맨드 전체 동작. 실제 클라이언트로 "채널 생성 → 초대 → 킥 → 모드변경" 시나리오 끝까지 통과.

---

## Phase 3 — 테스트 / 안정화 / 문서화 (Day 15~21)

### Day 15~17: 통합 테스트 & 버그픽스

- [ ] 크로스 테스트: 3인 각자 클라이언트로 동시 접속 (멀티채널, 동시 메시지, 강제 킥 등)
- [ ] 스트레스 테스트: 대량 연결, flood, 긴 메시지, 패킷 조각(fragmented) 처리
- [ ] fd/메모리 누수 점검: `valgrind --leak-check=full`
- [ ] norminette 검사

### Day 18~19: 리팩토링 & 코드리뷰

- [ ] 전원 코드리뷰
- [ ] 중복 로직 정리, 에러 처리 일관성 확보

### Day 20~21: 문서화 & 디펜스 준비

- [ ] README 최종화, 담당 파트별 설명 자료 준비
- [ ] 예상 질문 대비 (예: select 대신 poll 선택 이유, SIGPIPE 미처리 시 문제, 메시지 경계 처리 방식)
- [ ] 여유 시 보너스 착수 (mandatory 완성이 항상 우선)

---

## 협업 팁

- 매일 15분 스탠드업(또는 비동기 텍스트 공유): 어제 한 일 / 오늘 할 일 / 블로커
- Phase 1은 병렬성이 가장 높은 구간 — 최대한 활용
- 매주 최소 1회 전원이 poll() 루프를 로컬에서 함께 돌려보며 전체 흐름 검증
- 인터페이스 변경은 항상 사전 공지 후 진행