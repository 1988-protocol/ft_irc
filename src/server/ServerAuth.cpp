#include "server/Server.hpp"
#include "common/Utils.hpp"

const std::string& Server::getPassword() const
{
    return m_password;
}

bool Server::isNicknameInUse(const std::string& nickname) const
{
    if (nickname.empty())
        return false;
    for (std::map<int, Client*>::const_iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->second && Utils::isSameNickname(it->second->getNickname(), nickname))
            return true;
    }
    return false;
}
