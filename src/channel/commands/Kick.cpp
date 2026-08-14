#include "channel/commands/Kick.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

//RFC 1459 4.2.8

Kick::Kick() : ICommand() {}

Kick::~Kick() {}

void Kick::execute(Server& server, Client& client, const Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string target = client.getNickname();

    // 인자 개수 검사
    if (params.size() < 2)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "KICK :Not enough parameters"));
        return;
    }

    std::vector<std::string> channelNames = Utils::split(params[0], ',');
    std::vector<std::string> kickedNicks  = Utils::split(params[1], ',');

    // 강퇴 사유가 있을 경우 조립
    std::string reason = client.getNickname();
    if (msg.hasTrailing())
        reason = msg.getTrailing();
    else if (params.size() >= 3)
        reason = params[2];

    // 강퇴 유저 목록 루프 순회
    for (size_t i = 0; i < channelNames.size(); ++i)
    {
        std::string channelName = channelNames[i];
        std::string kickedNick = "";

        if (kickedNicks.size() > i)
            kickedNick = kickedNicks[i];
        else if (kickedNicks.size() == 1)
            kickedNick = kickedNicks[0];
        else
            break;

        // 채널 존재 여부 확인
        Channel* channel = server.getChannel(channelName);
        if (!channel)
        {
            client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
            continue;
        }

        // Kick을 실행한 유저가 채널 멤버인지 확인
        if (!channel->isMember(&client))
        {
            client.appendToOutBuffer(reply(Numeric::ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
            continue;
        }

        // Kick을 실행한 유저가 방장인지 확인
        if (!channel->isOperator(&client))
        {
            client.appendToOutBuffer(reply(Numeric::ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
            continue;
        }

        // Kick 시킬 유저 존재 및 채널 참가 여부 확인
        Client* kickedClient = server.getClientByNick(kickedNick);
        if (!kickedClient || !channel->isMember(kickedClient))
        {
            client.appendToOutBuffer(reply(Numeric::ERR_USERNOTINCHANNEL, target, kickedNick + " " + channelName + " :They aren't on that channel"));
            continue;
        }  

        // 강퇴 메시지 전송 (KICK 당한 유저 포함 전체 브로드캐스트)
        const std::map<Client*, bool>& members = channel->getMembers();
        for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            it->first->appendToOutBuffer(buildMessage(client, "KICK", channelName + " " + kickedNick, reason));
        }

        // 채널에서 Kick 당한 유저 제거
        channel->removeMember(kickedClient);

        // Kick 당한 유저가 채널에 남은 마지막 멤버였을 경우 채널 삭제
        if (channel->getMembers().empty())
        {
            server.removeChannel(channelName);
        }
    }
}
