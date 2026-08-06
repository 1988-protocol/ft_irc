#include "server/Server.hpp"
#include "common/Utils.hpp"

const std::string& Server::getPassword() const
{
    return m_password;
}

bool Server::isNicknameInUse(const std::string& nickname) const
{
    // 닉네임 점유 상태와 클라이언트 객체 상태간 차이 방어를 위한 코드인데, 좀 과한 것 같아서 잠시 보류. 
    // 좀 더 알아보기
    // for (std::map<std::string, Client*>::const_iterator it = m_nicknames.begin();
    //     it != m_nicknames.end(); ++it)
    // {
    //     if (it->second != NULL && Utils::isSameNickname(it->first, nickname))
    //         return true;
    // }
    for (std::map<int, Client*>::const_iterator it = m_clients.begin();
        it != m_clients.end(); ++it)
    {
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
