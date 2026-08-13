#include "channel/commands/Part.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

//RFC 1459 4.2.2

Part::Part() : ICommand() {}

Part::~Part() {}

void Part::execute(Server& server, Client& client, const Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string target = client.getNickname();

    // 인자 개수 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "PART :Not enough parameters"));
        return;
    }

    // 채널 목록 파싱
    std::vector<std::string> channelNames = Utils::split(params[0], ',');
    for (size_t i = 0; i < channelNames.size(); i++)
    {
        std::string channelName = channelNames[i];
        

        // 채널 존재 여부 확인
        Channel* channel = server.getChannel(channelName);
        if (!channel)
        {
            client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
            continue;
        }

        // 유저가 채널 멤버인지 확인
        if (!channel->isMember(&client))
        {
            client.appendToOutBuffer(reply(Numeric::ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
            continue;
        }

        // 퇴장 메시지 조립
        std::string reason = "";
        if (msg.hasTrailing())
            reason = msg.getTrailing();
        else if (params.size() >= 2)
            reason = params[1];

        // PART 메시지 브로드캐스트 (나가는 유저 포함 전원에게 전송)
        const std::map<Client*, bool>& members = channel->getMembers();
        for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            it->first->appendToOutBuffer(buildMessage(client, "PART", channelName, reason));
        }

        // 채널 유저 목록에서 제거
        channel->removeMember(&client);

        // Part한 멤버가 마지막 멤버였을 경우 빈 방 삭제 처리
        if (channel->getMembers().empty())
        {
            server.removeChannel(channelName);
        }
    }
}