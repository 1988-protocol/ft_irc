#include "channel/commands/Invite.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

//RFC 1459 4.2.7

Invite::Invite() : ICommand() {}

Invite::~Invite() {}

void Invite::execute(Server& server, Client& client, const Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string target = client.getNickname();

    // 인자 개수 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "INVITE :Not enough parameters"));
        return;
    }

    // 초대하고자 하는 유저의 nickname
    std::string invitedNick = params[0];
    std::string channelName = "";
    if (params.size() >= 2)
        channelName = params[1];
    // trailing 인자로 들어올 수도 있음
    else if (msg.hasTrailing())
        channelName = msg.getTrailing();
    if (channelName.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "INVITE :Not enough parameters"));
        return;
    }

    // 채널 존재 여부 검사
    Channel* channel = server.getChannel(channelName);
    if (!channel)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
        return;
    }

    // 초대한 유저가 해당 채널 멤버인지 검사
    if (!channel->isMember(&client))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
        return;
    }

    // +i 모드 일 때는 방장만 초대 가능
    if (channel->isInviteOnly() && !channel->isOperator(&client))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
        return;
    }

    // 초대 당할 대상 유저 존재 여부 검사
    Client* invitedClient = server.getClientByNick(invitedNick);
    if (!invitedClient)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHNICK, target, invitedNick + " :No such nick/channel"));
        return;
    }

    // 초대 대상 유저가 이미 채널에 있는지 검사
    if (channel->isMember(invitedClient))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_USERONCHANNEL, target, invitedNick + " " + channelName + " :is already on channel"));
        return;
    }

    // 채널 초대 목록에 초대 된 유저 추가
    channel->addInvite(invitedClient);

    // Invite 초대를 보낸 유저에게 성공 응답 전송
    client.appendToOutBuffer(reply(Numeric::RPL_INVITING, target, invitedNick + " " + channelName));

    // 초대받는 Invited 유저에게 INVITE 알림 전송
   invitedClient->appendToOutBuffer(buildMessage(client, "INVITE", invitedNick, channelName));
}