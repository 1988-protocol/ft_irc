#include "channel/commands/Invite.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

Invite::Invite() : ICommand() {}

Invite::~Invite() {}

void Invite::execute(Server& server, Client& client, const Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string clientNick = client.getNickname();

    // 1. params의 인자 개수 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, clientNick, "INVITE :Not enough parameters"));
        return;
    }

    // 명령어 : INVITE / 파라미터: <nickname> <channel> (초대할 유저의 닉네임과 대상 채널 이름)
    // 인자갯수에 문제 없으면 params의 가장 첫번째 인자가 초대하고자 하는 User의 nickname
    std::string targetNick = params[0];
    // 채널이름은 params의 두번째 인자로 올 수도 있고,
    std::string channelName = "";
    if (params.size() >= 2)
        channelName = params[1];
    // trailing 인자로 들어올 수도 있음
    else if (msg.hasTrailing())
        channelName = msg.getTrailing();
    // 들어온 채널이름 값이 없으면 에러메시지 전송
    if (channelName.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, clientNick, "INVITE :Not enough parameters"));
        return;
    }

    // 2. 채널 존재 여부 검사
    Channel* channel = server.getChannel(channelName);
    if (!channel)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, clientNick, channelName + " :No such channel"));
        return;
    }

    // 3. 초대한 주체(명령어를 실행한 client)가 해당 채널 멤버인지 검사
    if (!channel->isMember(&client))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOTONCHANNEL, clientNick, channelName + " :You're not on that channel"));
        return;
    }

    // 4. +i (초대 전용 모드) 일 때는 방장만 초대 가능
    if (channel->isInviteOnly() && !channel->isOperator(&client))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_CHANOPRIVSNEEDED, clientNick, channelName + " :You're not channel operator"));
        return;
    }

    // 5. 초대 당할 대상 유저 존재 여부 검사
    Client* targetClient = server.getClientByNick(targetNick);
    if (!targetClient)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHNICK, clientNick, targetNick + " :No such nick/channel"));
        return;
    }

    // 6. 초대 대상 유저가 이미 채널에 있는지 검사
    if (channel->isMember(targetClient))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_USERONCHANNEL, clientNick, targetNick + " " + channelName + " :is already on channel"));
        return;
    }

    // 7. 채널 초대 목록에 Client 포인터 객체 추가
    channel->addInvite(targetClient);

    // 8. 보낸 사람에게 성공 응답 전송
    client.appendToOutBuffer(reply(Numeric::RPL_INVITING, clientNick, targetNick + " " + channelName));

    // 9. 초대받는 타겟 유저에게 INVITE 알림 전송
   targetClient->appendToOutBuffer(buildMessage(client, "INVITE", targetNick, channelName));
}