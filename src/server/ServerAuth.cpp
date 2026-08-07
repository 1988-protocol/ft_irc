#include "server/Server.hpp"
#include "common/Utils.hpp"

const std::string& Server::getPassword() const
{
    return m_password;
}

bool Server::isNicknameInUse(const std::string& nickname) const
{
    return m_nicknames.find(Utils::toIRCLower(nickname)) != m_nicknames.end();
}

void Server::registerNickname(const std::string& nickname, Client& client)
{
    m_nicknames[Utils::toIRCLower(nickname)] = &client;
}

void Server::releaseNickname(const std::string& nickname)
{
    m_nicknames.erase(Utils::toIRCLower(nickname));
}
