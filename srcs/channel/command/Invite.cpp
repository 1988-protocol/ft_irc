#include "Invite.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Invite::Invite(Server* server) : ACommand(server) {}

Invite::Invite(const Invite& other) : ACommand(other) {}

Invite& Invite::operator=(const Invite& other)
{
    if (this != &other)
    {
        ACommand::operator=(other);
    }
    return *this;
}

Invite::~Invite() {}

void Invite::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    if (params.size() < 2)
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 461 " + sender->getNickname() + " INVITE :Not enough parameters\r\n");
        return;
    }

    std::string targetNick = params[0];
    std::string channelName = params[1];

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

    // +i (초대 전용 모드) 일 때는 방장만 가능
    if (channel->isInviteOnly() && !channel->isOperator(sender))
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 482 " + sender->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    Client* target = _server->getClientByNick(targetNick);
    if (!target)
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 401 " + sender->getNickname() + " " + targetNick + " :No such nick/channel\r\n");
        return;
    }

    if (channel->isUserInChannel(target))
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 443 " + sender->getNickname() + " " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }

    channel->addInvite(targetNick);

    // 보낸 사람에게 성공 응답(341) 및 타겟 유저에게 초대 알림 전송
    _server->sendToClient(sender->getFd(), ":ft_irc 341 " + sender->getNickname() + " " + targetNick + " " + channelName + "\r\n");
    _server->sendToClient(target->getFd(), ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname() + " INVITE " + targetNick + " :" + channelName + "\r\n");
}