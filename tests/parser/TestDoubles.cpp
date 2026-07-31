#include "client/Client.hpp"
#include "server/Server.hpp"

// ============================================================================
// 테스트 전용 구현체 — 실제 Network가 작성할 irc/srcs/client/Client.cpp,
// irc/srcs/server/Server.cpp를 대체하지 않는다. irc/tests/parser 타겟에서만 링크되며,
// 실제 ircserv 바이너리 빌드에는 절대 포함되지 않는다.
// pre_plan.md Phase1 "PASS/NICK/USER 등록 시퀀스... 테스트: mock Client로 파싱 단위
// 테스트" 요구사항을 이 파일로 충족한다. Client/Server는 include/client, include/server의
// 선언(Parser의 제안 초안)을 그대로 따르며, 여기서는 그 몸체만 테스트용으로 채운다.
// ============================================================================

Client::Client()
    : m_registered(false), m_hasCorrectPassword(false)
{
}

bool Client::isRegistered() const { return m_registered; }
void Client::setRegistered(bool value) { m_registered = value; }

bool Client::hasCorrectPassword() const { return m_hasCorrectPassword; }
void Client::setHasCorrectPassword(bool value) { m_hasCorrectPassword = value; }

const std::string& Client::getNickname() const { return m_nickname; }
void Client::setNickname(const std::string& nickname) { m_nickname = nickname; }

const std::string& Client::getUsername() const { return m_username; }
void Client::setUsername(const std::string& username) { m_username = username; }

void Client::appendToOutBuffer(const std::string& line) { m_outbox += line; }
const std::string& Client::getOutbox() const { return m_outbox; }
void Client::clearOutbox() { m_outbox.clear(); }

Server::Server(const std::string& password)
    : m_password(password)
{
}

bool Server::isNicknameInUse(const std::string& nickname) const
{
    return m_nicknames.find(nickname) != m_nicknames.end();
}

void Server::registerNickname(const std::string& nickname, Client& client)
{
    m_nicknames[nickname] = &client;
}

void Server::releaseNickname(const std::string& nickname)
{
    m_nicknames.erase(nickname);
}

const std::string& Server::getPassword() const { return m_password; }
