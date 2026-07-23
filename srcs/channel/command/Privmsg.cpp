#include "Privmsg.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Privmsg::Privmsg(Server* server) : ACommand(server) {}

Privmsg::Privmsg(const Privmsg& other) : ACommand(other) {}

Privmsg& Privmsg::operator=(const Privmsg& other)
{
    if (this != &other)
    {
        ACommand::operator=(other);
    }
    return *this;
}

Privmsg::~Privmsg() {}

void Privmsg::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    if (params.size() < 2)
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 461 " + sender->getNickname() + " PRIVMSG :Not enough parameters\r\n");
        return;
    }

    std::string targetName = params[0];
    std::string message = params[1];

    // 수신 대상이 채널인 경우 (#으로 시작)
    if (targetName[0] == '#')
    {
        Channel* channel = _server->getChannel(targetName);
        if (!channel)
        {
            _server->sendToClient(sender->getFd(), ":ft_irc 401 " + sender->getNickname() + " " + targetName + " :No such nick/channel\r\n");
            return;
        }

        if (!channel->isUserInChannel(sender))
        {
            _server->sendToClient(sender->getFd(), ":ft_irc 404 " + sender->getNickname() + " " + targetName + " :Cannot send to channel\r\n");
            return;
        }

        std::string packet = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                           + " PRIVMSG " + targetName + " :" + message + "\r\n";

        const std::vector<Client*>& users = channel->getUsers();
        for (size_t i = 0; i < users.size(); ++i)
        {
            if (users[i] != sender) // 나를 제외한 방 멤버들에게만 전송
            {
                _server->sendToClient(users[i]->getFd(), packet);
            }
        }
    }
    // (선택) 개인 DM 대상인 경우 생략 또는 추가 구현
}