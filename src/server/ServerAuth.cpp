#include "server/Server.hpp"
#include "common/Utils.hpp"

const std::string& Server::getPassword() const
{
    return m_password;
}

bool Server::isNicknameInUse(const std::string& nickname) const
{
    for (std::map<int, Client*>::const_iterator it = m_clients.begin();
        it != m_clients.end(); ++it)
    {
        // nickname 중복 함수를 거치도록 수정했습니다.
        if (it->second != NULL && Utils::isSameNickname(it->second->getNickname(), nickname))
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
