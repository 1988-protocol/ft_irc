#include "Privmsg.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Privmsg::Privmsg(Server* server) : ICommand(server) {}

Privmsg::Privmsg(const Privmsg& other) : ICommand(other) {}

Privmsg& Privmsg::operator=(const Privmsg& other)
{
    if (this != &other)
    {
        ICommand::operator=(other);
    }
    return *this;
}

Privmsg::~Privmsg() {}

void Privmsg::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    // 1. 수신자 미지정 검사 (411 ERRm_NORECIPIENT)
    if (params.empty())
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 411 " + sender->getNickname() + " :No recipient given (PRIVMSG)\r\n");
        return;
    }

    // 2. 메시지 내용 누락 검사 (412 ERRm_NOTEXTTOSEND)
    if (params.size() < 2 || params[1].empty())
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 412 " + sender->getNickname() + " :No text to send\r\n");
        return;
    }

    std::string targetName = params[0];
    std::string message = params[1];

    std::string packet = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                       + " PRIVMSG " + targetName + " :" + message + "\r\n";

    // 3. 수신 대상이 채널인 경우 ('#'으로 시작)
    if (!targetName.empty() && targetName[0] == '#')
    {
        Channel* channel = m_server->getChannel(targetName);
        
        // 채널 존재 여부 검사 (401 ERRm_NOSUCHNICK)
        if (!channel)
        {
            m_server->sendToClient(sender->getFd(), ":ftm_irc 401 " + sender->getNickname() + " " + targetName + " :No such nick/channel\r\n");
            return;
        }

        // 보낸 유저가 채널 멤버인지 검사 (404 ERRm_CANNOTSENDTOCHAN)
        if (!channel->isUserInChannel(sender))
        {
            m_server->sendToClient(sender->getFd(), ":ftm_irc 404 " + sender->getNickname() + " " + targetName + " :Cannot send to channel\r\n");
            return;
        }

        // 나(sender)를 제외한 채널 내 모든 멤버에게 메시지 전송
        const std::vector<Client*>& users = channel->getUsers();
        for (sizem_t i = 0; i < users.size(); ++i)
        {
            if (users[i] != sender)
            {
                m_server->sendToClient(users[i]->getFd(), packet);
            }
        }
    }
    // 4. 수신 대상이 개인 유저인 경우 (1:1 PRIVMSG)
    else
    {
        Client* targetClient = m_server->getClientByNick(targetName);
        
        // 유저가 존재하지 않는 경우 (401 ERRm_NOSUCHNICK)
        if (!targetClient)
        {
            m_server->sendToClient(sender->getFd(), ":ftm_irc 401 " + sender->getNickname() + " " + targetName + " :No such nick/channel\r\n");
            return;
        }

        // 1:1 메시지 전송
        m_server->sendToClient(targetClient->getFd(), packet);
    }
}