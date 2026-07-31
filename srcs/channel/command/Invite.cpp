#include "Invite.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Invite::Invite(Server* server) : ICommand(server) {}

Invite::Invite(const Invite& other) : ICommand(other) {}

Invite& Invite::operator=(const Invite& other)
{
    if (this != &other)
    {
        ICommand::operator=(other);
    }
    return *this;
}

Invite::~Invite() {}

void Invite::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    // 1. 인자 개수 검사 (461 ERRm_NEEDMOREPARAMS)
    if (params.size() < 2)
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 461 " + sender->getNickname() + " INVITE :Not enough parameters\r\n");
        return;
    }

    std::string targetNick = params[0];
    std::string channelName = params[1];

    // 2. 채널 존재 여부 검사 (403 ERRm_NOSUCHCHANNEL)
    Channel* channel = m_server->getChannel(channelName);
    if (!channel)
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 403 " + sender->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    // 3. 초대한 사람이 해당 채널 멤버인지 검사 (442 ERRm_NOTONCHANNEL)
    if (!channel->isUserInChannel(sender))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 442 " + sender->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // 4. +i (초대 전용 모드) 일 때는 방장만 초대 가능 (482 ERRm_CHANOPRIVSNEEDED)
    if (channel->isInviteOnly() && !channel->isOperator(sender))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 482 " + sender->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    // 5. 초대 대상 유저 존재 여부 검사 (401 ERRm_NOSUCHNICK)
    Client* target = m_server->getClientByNick(targetNick);
    if (!target)
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 401 " + sender->getNickname() + " " + targetNick + " :No such nick/channel\r\n");
        return;
    }

    // 6. 초대 대상 유저가 이미 채널에 있는지 검사 (443 ERRm_USERONCHANNEL)
    if (channel->isUserInChannel(target))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 443 " + sender->getNickname() + " " + targetNick + " " + channelName + " :is already on channel\r\n");
        return;
    }

    // 7. 🌟 채널 초대 목록에 Client 포인터 객체 추가
    channel->addInvite(target);

    // 8. 보낸 사람에게 성공 응답(341 RPLm_INVITING)전송
    m_server->sendToClient(sender->getFd(), ":ftm_irc 341 " + sender->getNickname() + " " + targetNick + " " + channelName + "\r\n");

    // 9. 초대받는 타겟 유저에게 INVITE 알림 전송
    m_server->sendToClient(target->getFd(), ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname() 
                                         + " INVITE " + targetNick + " :" + channelName + "\r\n");
}