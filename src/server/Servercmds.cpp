#include "server/Server.hpp"
#include "channel/Channel.hpp"
#include "common/Utils.hpp"

Channel* Server::getChannel(const std::string& channelName)
{
    std::map<std::string, Channel*>::iterator it = m_channels.find(channelName);
    if (it != m_channels.end())
        return it->second;
    return NULL;
}

void Server::addChannel(const std::string& channelName, Channel* channel)
{
    m_channels[channelName] = channel;
}

void Server::removeChannel(const std::string& channelName)
{
    m_channels.erase(channelName);
}

// [changed 0807] PRIVMSG alice :hello를 보내면 "Alice"로 등록한 사용자를 찾지 못하는 문제 개선
// 이전 if (it->second && it->second->getNickname() == nickname)
// 변경 후 if (it->second && Utils::isSameNickname(it->second->getNickname(), nickname))
// nick을 가져오는데 대소문자 구분 없이 비교를 하고 가져옵니다.
Client* Server::getClientByNick(const std::string& nickname)
{
    for (std::map<int, Client*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->second && Utils::isSameNickname(it->second->getNickname(), nickname))
            return it->second;
    }
    return NULL;
}