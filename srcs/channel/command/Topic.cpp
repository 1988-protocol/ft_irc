#include "Topic.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Topic::Topic(Server* server) : ICommand(server) {}

Topic::Topic(const Topic& other) : ICommand(other) {}

Topic& Topic::operator=(const Topic& other)
{
    if (this != &other)
    {
        ICommand::operator=(other);
    }
    return *this;
}

Topic::~Topic() {}

void Topic::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    // 1. 인자 개수 검사 (461 ERRm_NEEDMOREPARAMS)
    if (params.empty())
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 461 " + sender->getNickname() + " TOPIC :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];

    // 2. 채널 존재 여부 확인 (403 ERRm_NOSUCHCHANNEL)
    Channel* channel = m_server->getChannel(channelName);
    if (!channel)
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 403 " + sender->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    // 3. 요청 유저가 채널 멤버인지 확인 (442 ERRm_NOTONCHANNEL)
    if (!channel->isUserInChannel(sender))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 442 " + sender->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // 4. [단순 조회 요청] 인자가 채널명 1개일 때
    if (params.size() == 1)
    {
        if (channel->getTopic().empty())
        {
            m_server->sendToClient(sender->getFd(), ":ftm_irc 331 " + sender->getNickname() + " " + channelName + " :No topic is set\r\n");
        }
        else
        {
            m_server->sendToClient(sender->getFd(), ":ftm_irc 332 " + sender->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
        }
        return;
    }

    // 5. [토픽 변경 요청] +t 모드 권한 검사 (482 ERRm_CHANOPRIVSNEEDED)
    if (channel->isTopicOpOnly() && !channel->isOperator(sender))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 482 " + sender->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    // 6. 토픽 변경 및 브로드캐스트
    std::string newTopic = params[1];
    channel->setTopic(newTopic);

    std::string topicMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                             + " TOPIC " + channelName + " :" + newTopic + "\r\n";

    const std::vector<Client*>& users = channel->getUsers();
    for (sizem_t i = 0; i < users.size(); ++i)
    {
        m_server->sendToClient(users[i]->getFd(), topicMessage);
    }
}