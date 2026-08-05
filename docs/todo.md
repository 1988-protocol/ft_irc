# phase00 합의할 것

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

# 현재 수행한 것
*(세부 계획 및 전체 일정은 [pre_plan.md](./pre_plan.md) 참고)*

## Phase 1 — 개별 기능 뼈대 (Day 3~7)

### Parser

- [ ] Message 파싱 (prefix / command / params / trailing)
  - 엣지 케이스: 중복 공백, 빈 파라미터, trailing 없는 경우
- [ ] 커맨드 dispatcher 골격 (`std::map<std::string, HandlerFunc>` 등)
- [ ] PASS/NICK/USER 등록 시퀀스, 미등록 상태에서 타 커맨드 거부
- [ ] 에러 응답 코드 정리 (431, 432, 433, 451, 461 등)
- [ ] 테스트: mock Client로 파싱 단위 테스트

---

## 참고: `.gitkeep` 개념

- Git은 기본적으로 **빈 디렉토리(폴더)를 추적하지 않음**. 폴더 내부에 최소 1개의 파일이 존재해야 Git 저장소에 등록됨.
- 아직 코드가 없는 빈 디렉토리 구조(예: `include/server/`, `src/channel/commands/` 등)를 저장소에 유지하고 공유하기 위해 생성하는 **임시 빈 파일**.
- Git 공식 예약어는 아니며, 개발자들 사이의 **관례(Convention)**적 명칭.
- 해당 디렉토리에 실제 구현 파일(`.cpp`, `.hpp` 등)이 추가되면 **삭제 가능**.