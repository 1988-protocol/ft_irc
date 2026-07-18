#ifndef CLIENT_HPP
#define CLIENT_HPP

// ============================================================================
// 제안 — 팀 합의 필요
// README.md 기준 Client는 Network 소유(irc/srcs/client/Client.cpp에서 구현)이며,
// 이 헤더는 Parser의 PASS/NICK/USER 등록 시퀀스가 필요로 하는 최소 메서드만 담아
// Parser 담당자가 제안한 초안이다. fd/poll 관련 멤버(소켓 fd, 송수신 버퍼 등)는
// Network가 실제 구현 시 추가로 필요하겠지만, 그 부분은 Parser가 알 필요도 결정할
// 권한도 없어 여기 넣지 않았다. 최종 시그니처는 팀 회의에서 Network 담당자와 확정한다.
// ============================================================================

#include <string>

class Client
{
public:
    Client();

    // PASS -> NICK -> USER 세 값이 모두 채워졌는지로 등록 완료를 판단하는 대신,
    // 등록 완료 시점(마지막으로 채워지는 값을 받은 커맨드)에 한 번만 true로 세팅되는
    // 명시적 플래그를 둔다 — 판단 로직을 여러 커맨드 클래스에 중복시키지 않기 위함.
    bool isRegistered() const;
    void setRegistered(bool value);

    bool hasCorrectPassword() const;
    void setHasCorrectPassword(bool value);

    const std::string& getNickname() const;
    void setNickname(const std::string& nickname);

    const std::string& getUsername() const;
    void setUsername(const std::string& username);

    // 레이어 경계: 모든 레이어가 이 클라이언트에게 응답을 내보내는 유일한 통로.
    // 실제 send()는 Network가 poll()에서 이 fd가 writable일 때 outbox를 읽어 flush한다
    // (mini_serv.c의 outbuf/flush_client와 동일한 사상). Client는 fd를 모른다 —
    // fd<->Client 매핑은 Network가 별도로 관리한다.
    void queueReply(const std::string& line);
    const std::string& getOutbox() const;
    void clearOutbox();

private:
    bool m_registered;
    bool m_hasCorrectPassword;
    std::string m_nickname;
    std::string m_username;
    std::string m_outbox;
};

#endif
