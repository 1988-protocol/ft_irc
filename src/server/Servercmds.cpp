#include "server/Server.hpp"
#include "client/Client.hpp"
#include "channel/Channel.hpp"
#include "common/Utils.hpp"

Channel* Server::getChannel(const std::string& channelName)
{
    std::map<std::string, Channel*>::iterator it = m_channels.find(Utils::toIRCLower(channelName));
    if (it != m_channels.end())
        return it->second;
    return NULL;
}

const std::map<std::string, Channel*>& Server::getChannels() const
{
    return m_channels;
}

void Server::addChannel(const std::string& channelName, Channel* channel)
{
    m_channels[Utils::toIRCLower(channelName)] = channel;
}

void Server::removeChannel(const std::string& channelName)
{
    std::map<std::string, Channel*>::iterator it = m_channels.find(Utils::toIRCLower(channelName));
    if (it != m_channels.end())
    {
        // Server가 객체 메모리 해제
        delete it->second;
        // map 항목 삭제
        m_channels.erase(it);
    }
}

Client* Server::getClientByNick(const std::string& nickname)
{
    if (nickname.empty())
        return NULL;
    for (std::map<int, Client*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->second && Utils::isSameNickname(it->second->getNickname(), nickname))
            return it->second;
    }
    return NULL;
}

// 유저가 참여한 채널 개수 세는 함수
// Server가 갖고 있는 m_channels을 순회하며 해당 클라이언트가 들어가 있는 채널 수를 카운팅
size_t Server::getUserJoinedChannelCount(Client* client) const
{
    size_t count = 0;
    for (std::map<std::string, Channel*>::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it)
    {
        if (it->second && it->second->isMember(client))
            count++;
    }
    return count;
}
