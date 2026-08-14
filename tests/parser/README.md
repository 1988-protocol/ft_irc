# Parser 단위 테스트 (`test_parser.cpp`) 가이드

[`test_parser.cpp`](file:///home/borlee/assignment/team_irc/tests/parser/test_parser.cpp)에 포함된 모든 명령어와 테스트 케이스별 입력 및 출력 결과에 대한 상세 설명은 [`docs/PARSER_TEST_GUIDE.md`](file:///home/borlee/assignment/team_irc/docs/PARSER_TEST_GUIDE.md)에 정리되어 있습니다.

## 🚀 빠른 실행 (Quick Start)

### 전체 테스트 실행
```bash
make test
```

### 파서 단위 테스트 단독 실행
```bash
c++ -Wall -Wextra -Werror -std=c++98 -I include tests/parser/test_parser.cpp src/common/*.cpp src/server/*.cpp src/client/*.cpp src/parser/*.cpp src/parser/commands/*.cpp src/channel/*.cpp src/channel/commands/*.cpp -o test_parser && ./test_parser
```

## 📋 수록된 명령어 및 파싱 가이드
- **NICK**: 닉네임 설정, 431/432/433 에러, Irssi 호환 문자(`_`, `|`), 중복 닉네임 처리
- **PASS**: 비밀번호 인증, 461/464/462 에러, 인증 플래그 처리
- **USER**: 사용자 정보 등록, 461/462 에러, 4개 파라미터 및 trailing 처리
- **[등록]**: 6가지 순서 조합 및 `001 RPL_WELCOME` 환영 응답
- **PING / PONG**: 토큰 에코, 409/402 에러, 서버명 대소문자 무시, 무응답(no-op) 규칙
- **QUIT**: 종료 사유 trailing, `ERROR :Closing Link` 전송 및 정상 세션 종료
- **Message 파싱**: RFC 1459 문법 및 15개 엣지 케이스 (중복 공백, 탭, 15번째 파라미터 trailing, NUL 차단)
- **오류 복구**: 오타/에러 발생 시 세션 유지 및 재입력 복구 시나리오
