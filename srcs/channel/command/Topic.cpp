#include "Topic.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Topic::Topic(Server* server) : ACommand(server) {}

Topic::Topic(const Topic& other) : ACommand(other) {}

Topic& Topic::operator=(const Topic& other)
{
    if (this != &other)
    {
        ACommand::operator=(other);
    }
    return *this;
}

Topic::~Topic() {}

void Topic::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    if (params.empty())
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 461 " + sender->getNickname() + " TOPIC :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];

    Channel* channel = _server->getChannel(channelName);
    if (!channel)
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 403 " + sender->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    if (!channel->isUserInChannel(sender))
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 442 " + sender->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // [단순 조회 요청] : 인자가 1개일 때
    if (params.size() == 1)
    {
        if (channel->getTopic().empty())
        {
            _server->sendToClient(sender->getFd(), ":ft_irc 331 " + sender->getNickname() + " " + channelName + " :No topic is set\r\n");
        }
        else
        {
            _server->sendToClient(sender->getFd(), ":ft_irc 332 " + sender->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
        }
        return;
    }

    // [토픽 변경 요청] : 인자가 2개 이상일 때
    std::string newTopic = params[1];

    // +t 모드(토픽 제한)이고 방장이 아니면 차단
    if (channel->isTopicProtected() && !channel->isOperator(sender))
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 482 " + sender->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    channel->setTopic(newTopic);

    std::string topicMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                             + " TOPIC " + channelName + " :" + newTopic + "\r\n";

    const std::vector<Client*>& users = channel->getUsers();
    for (size_t i = 0; i < users.size(); ++i)
    {
        _server->sendToClient(users[i]->getFd(), topicMessage);
    }
}