#include "server/Server.hpp"
#include "common/Utils.hpp"

const std::string& Server::getPassword() const
{
    return m_password;
}

bool Server::isNicknameInUse(const std::string& nickname) const
{
    for (std::map<std::string, Client*>::const_iterator it = m_nicknames.begin();
        it != m_nicknames.end(); ++it)
    {
        if (it->second != NULL && Utils::isSameNickname(it->first, nickname))
            return true;
    }
    return false;
}

void Server::registerNickname(const std::string& nickname, Client& client)
{
    m_nicknames[nickname] = &client;
}

void Server::releaseNickname(const std::string& nickname)
{
    m_nicknames.erase(nickname);
}
