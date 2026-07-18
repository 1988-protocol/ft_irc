#ifndef SERVER_HPP
#define SERVER_HPP

// ============================================================================
// 제안 — 팀 합의 필요
// README.md 기준 Server는 Network 소유(PollManager/Socket과 함께 irc/srcs/server/에서
// 구현)이며, 이 헤더는 Parser의 NICK/PASS 처리가 필요로 하는 3개 메서드만 담은 초안이다.
// socket/bind/listen/accept/poll 관련 멤버나 메서드는 전적으로 Network 담당이라
// 여기서 다루지 않는다. 최종 시그니처는 팀 회의에서 확정한다.
// ============================================================================

#include <map>
#include <string>

class Client;

class Server
{
public:
    // README의 "./ircserv <port> <password>" 실행 방식에 맞춰 비밀번호를 생성 시점에
    // 받는다고 가정한 최소 생성자. port/소켓 관련 인자는 Network 몫이라 여기 넣지 않았다.
    Server(const std::string& password);

    // 이미 다른 클라이언트가 쓰고 있는 닉네임인지 확인한다(NICK 커맨드의 433 판단).
    bool isNicknameInUse(const std::string& nickname) const;

    // 닉네임을 이 클라이언트 소유로 등록한다. 이미 등록 로직(isNicknameInUse)을
    // 통과했다는 전제로 호출되며, 내부 상태(서버가 갖는 닉네임<->Client 매핑)를 갱신한다.
    void registerNickname(const std::string& nickname, Client& client);

    // 제안 추가(2026-07-18, 리뷰에서 발견): registerNickname()은 새 닉네임을 추가만 하고
    // 이전 닉네임 엔트리를 지우지 않아, NICK으로 닉네임을 바꾸거나 QUIT으로 연결이
    // 끊겨도 예전 닉네임이 map에 영구히 남아 다른 클라이언트가 재사용할 수 없는 문제가
    // 있었다. 이 메서드로 특정 닉네임 엔트리를 제거한다 — Nick::execute(닉네임 변경 시
    // 이전 값 해제)와 Quit::execute(연결 종료 시 현재 값 해제)에서 사용한다. 팀 합의
    // 필요 — irc/md/parser_message_grammar.md 4.6 참고.
    void releaseNickname(const std::string& nickname);

    // PASS 커맨드가 클라이언트 입력과 비교할 서버 접속 비밀번호.
    const std::string& getPassword() const;

private:
    // 닉네임<->Client 매핑의 실제 저장 방식(자료구조, 대소문자 처리 등)은 Network의
    // 재량이다 — 여기서는 Parser의 테스트가 컴파일/동작하는 데 필요한 최소 형태만 제안한다.
    std::string m_password;
    std::map<std::string, Client*> m_nicknames;
};

#endif
