
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
    m_channels.erase(channelName);
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
