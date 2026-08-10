#include "channel/commands/Topic.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

Topic::Topic() : ICommand() {}

Topic::~Topic() {}

void Topic::execute(Server& server, Client& client, const Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string target = client.getNickname();

    // 모든 명령어 공통 params 인자 개수 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "TOPIC :Not enough parameters"));
        return;
    }

    // 명령어: TOPIC / 파라미터: <channel> [<topic>] (두 번째 인자인 topic은 생략 가능)
    std::string channelName = params[0];

    // 모든 명령어 공통 채널 존재 여부 확인
    Channel* channel = server.getChannel(channelName);
    if (!channel)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
        return;
    }

    // 3. 요청 유저가 채널 멤버인지 확인
    if (!channel->isMember(&client))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
        return;
    }

    // 4. [단순 조회 요청] 인자가 채널명 1개일 때
    if (params.size() == 1 && !msg.hasTrailing())
    {
        if (channel->getTopic().empty())
        {
            client.appendToOutBuffer(reply(Numeric::RPL_NOTOPIC, target, channelName + " :No topic is set"));
        }
        else
        {
            client.appendToOutBuffer(reply(Numeric::RPL_TOPIC, target, channelName + " :" + channel->getTopic()));
        }
        return;
    }

    // 5. [토픽 변경 요청] +t 모드 권한 검사
    if (channel->isTopicOpOnly() && !channel->isOperator(&client))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
        return;
    }

    // 6. 토픽 변경 및 브로드캐스트
    std::string newTopic = "";
    if (msg.hasTrailing())
        newTopic = msg.getTrailing();
    else if (params.size() >= 2)
        newTopic = params[1];
    else
        return;

    channel->setTopic(newTopic);

    const std::map<Client*, bool>& members = channel->getMembers();
    for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
    {
        it->first->appendToOutBuffer(buildMessage(client, "TOPIC", channelName, newTopic));
    }
}