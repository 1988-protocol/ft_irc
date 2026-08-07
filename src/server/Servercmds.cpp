#include "server/Server.hpp"
#include "channel/Channel.hpp"

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
    std::map<std::string, Channel*>::iterator it = m_channels.find(channelName);
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
    for (std::map<int, Client*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->second && it->second->getNickname() == nickname)
            return it->second;
    }
    return NULL;
}