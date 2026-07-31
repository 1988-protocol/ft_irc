#include "server/Server.hpp"

const std::string& Server::getPassword() const
{
    return m_password;
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