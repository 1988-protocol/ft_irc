#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>

#include "channel/Channel.hpp"

class Server
{
private:
    std::map<std::string, Channel*> m_channels;

public:

    Channel* getChannel(const std::string& channelName);
    void addChannel(const std::string& channelName, Channel* channel);
    void removeChannel(const std::string& channelName);
    Client* getClientByNick(const std::string& nickname);
};

#endif
