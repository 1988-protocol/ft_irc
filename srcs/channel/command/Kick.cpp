#include "channel/commands/Kick.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

Kick::Kick() : ICommand() {}

Kick::~Kick() {}

void Kick::execute(Server& server, Client& client, Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string target = client.getNickname();

    // 1. 인자 개수 검사
    if (params.size() < 2)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "KICK :Not enough parameters"));
        return;
    }

    // 명령어: KICK / 파라미터: <channel> <user> [<comment>] (채널 이름, 쫓아낼 유저, 강퇴 사유[생략 가능])
    std::string channelName = params[0];
    std::string targetNick = params[1];

    std::string reason = msg.hasTrailing() ? msg.getTrailing() : ((params.size() >= 3) ? params[2] : client.getNickname());

    // 2. 채널 존재 여부 확인
    Channel* channel = server.getChannel(channelName);
    if (!channel)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
        return;
    }

    // 3. 명령 내린 sender가 채널 멤버인지 확인
    if (!channel->isUserInChannel(&client))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
        return;
    }

    // 4. sender가 operator(방장)인지 확인
    if (!channel->isOperator(&client))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
        return;
    }

    // 5. 강퇴 대상(target) 존재 및 채널 참가 여부 확인
    Client* targetClient = server.getClientByNick(targetNick);
    if (!targetClient || !channel->isUserInChannel(targetClient))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_USERNOTINCHANNEL, target, targetNick + " " + channelName + " :They aren't on that channel"));
        return;
    }

    // 6. 강퇴 메시지 전송 (나가는 target 포함 전체 브로드캐스트)
    std::string kickMessage = ":" + target + "!" + client.getUsername() + "@" + client.getHostname()
                            + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n"; // 🌟 " :" 공백 수정
    
    const std::map<Client*, bool>& members = channel->getMembers();
    for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        it->first->appendToOutBuffer(kickMessage);
    }

    // 7. 채널에서 target 제거 (Channel.cpp 내부에서 방장/초대 목록 연쇄 정리)
    channel->removeUser(targetClient);

    // 8. 채널 소멸 검사 (빈 방 삭제)
    if (channel->getMembers().empty())
    {
        server.removeChannel(channelName);
        delete channel;
    }
}
