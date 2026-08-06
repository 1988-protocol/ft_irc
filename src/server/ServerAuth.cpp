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
        if (Utils::isSameNickname(it->second->getNickname(), nickname))
            return true;
    }
    return false;
}
