#include "channel/commands/Join.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

namespace
{
    bool isValidChannelName(const std::string& name)
    {
        if (name.empty() || name.size() > 200)
            return false;
        if (name[0] != '#' && name[0] != '&')
            return false;
        for (size_t i = 0; i < name.size(); ++i)
        {
            if (name[i] == '\a')
                return false;
        }
        return true;
    }
}

//RFC 1459 4.2.1

Join::Join() : ICommand() {}

Join::~Join() {}

void Join::execute(Server& server, Client& client, const Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string target = client.getNickname();

    // 인자 개수 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "JOIN :Not enough parameters"));
        return;
    }
    // 채널 목록 파싱
    std::vector<std::string> channelNames = Utils::split(params[0], ',');
    std::vector<std::string> keys;
    // 들어온 인자값이 2개 이상일 경우 key 값도 파싱
    if (params.size() >= 2)
        keys = Utils::split(params[1], ',');
    for (size_t i = 0; i < channelNames.size(); i++)
    {
        std::string channelName = channelNames[i];
        std::string inputKey = "";
        if (keys.size() > i)
            inputKey = keys[i];

        // 채널 이름 유효성 검사 (RFC 1459 1.3) 
        if (!isValidChannelName(channelName))
        {
            client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :Bad channel name"));
            continue;
        }

        // 채널 존재 여부 확인
        Channel* channel = server.getChannel(channelName);

        // 채널 갯수 10개 이하 권장 (RFC 1459 8. 13)
        if ((!channel || !channel->isMember(&client)) && server.getUserJoinedChannelCount(&client) >= 10)
        {
            client.appendToOutBuffer(reply(Numeric::ERR_TOOMANYCHANNELS, target, channelName + " :You have joined too many channels"));
            continue;
        }

        if (!channel)
        {
            // 처음 생성될 때는 채널 객체 생성 후 유저 추가 및 방장 지정
            channel = new Channel(channelName);
            server.addChannel(channelName, channel);
            channel->addMember(&client);
            channel->addOperator(&client);
        }
        else
        {
            // 이미 참가 중인 유저는 중복 진입 방지
            if (channel->isMember(&client))
                continue;

            // +i 모드 검사 (초대 전용 모드일 때 Join 하려는 유저가 방장으로부터 초대된 유저인지) 확인
            if (channel->isInviteOnly() && !channel->isInvited(&client))
            {
                client.appendToOutBuffer(reply(Numeric::ERR_INVITEONLYCHAN, target, channelName + " :Cannot join channel (+i)"));
                continue;
            }

            // +k 모드 검사 (채널 키 모드일 때 지정되어 있는 key값과 유저가 작성한 key 값이 일치하는지 확인)
            if (channel->isKeyModeActive() && !channel->checkKey(inputKey))
            {
                client.appendToOutBuffer(reply(Numeric::ERR_BADCHANNELKEY, target, channelName + " :Cannot join channel (+k)"));
                continue;
            }

            // +l 모드 검사 (채널 인원이 다 차있는지 확인)
            if (channel->isFull())
            {
                client.appendToOutBuffer(reply(Numeric::ERR_CHANNELISFULL, target, channelName + " :Cannot join channel (+l)"));
                continue;
            }

            // 최종 유저 추가
            channel->addMember(&client);
        }

        // 초대받아서 들어온 유저라면 초대여부 사용
        if (channel->isInvited(&client))
            channel->removeInvite(&client);

        // 입장 알림 브로드캐스트 (새 유저 포함 채널 내 모든 사람에게 전송)
        const std::map<Client*, bool>& members = channel->getMembers();
        for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            it->first->appendToOutBuffer(buildMessage(client, "JOIN", channelName, ""));
        }

        // 입장한 유저에게 Topic 전송
        if (channel->getTopic().empty())
        {
            client.appendToOutBuffer(reply(Numeric::RPL_NOTOPIC, target, channelName + " :No topic is set"));
        }
        else
        {
            client.appendToOutBuffer(reply(Numeric::RPL_TOPIC, target, channelName + " :" + channel->getTopic()));
        }

        // 입장한 유저에게 유저목록 전송
        std::string memberList = "";
        
        for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            // 1.3.1 channel operator (유저목록 속 방장은 @표기, 일반 유저는 공백)
            Client* member = it->first;
            std::string memberNick = member->getNickname();
        
            // 방장인 경우 닉네임 앞에 '@' 붙여야 함 (RFC 1459 1.3.1)
            if (channel->isOperator(member))
            {
                memberNick = "@" + memberNick;
            }

            // 닉네임을 덧붙였을 때 완성될 임시 닉네임목록 생성
            std::string tempList = memberNick;
            if (!memberList.empty())
            {
                tempList = memberList + " " + memberNick;
            }

            // 완성될 353 RPL_NAMREPLY 전체 프레임 생성 후 크기 측정
            std::string testReply = reply(Numeric::RPL_NAMREPLY, target, "= " + channelName + " :" + tempList);

            // 완성된 전체 메시지가 512바이트를 초과하면, 기존까지 모은 memberList를 먼저 전송
            if (!memberList.empty() && (testReply.size() > 512))
            {
                client.appendToOutBuffer(reply(Numeric::RPL_NAMREPLY, target, "= " + channelName + " :" + memberList));
                memberList = memberNick; // 새 닉네임부터 다시 모으기 시작
            }
            else
            {
                memberList = tempList;
            }
        }
        //memberList에 잔여 유저 목록 전송
        if (!memberList.empty())
        {
            client.appendToOutBuffer(reply(Numeric::RPL_NAMREPLY, target, "= " + channelName + " :" + memberList));
        }
        // 366 RPL_ENDOFNAMES(목록 전송 완료 신호)
        client.appendToOutBuffer(reply(Numeric::RPL_ENDOFNAMES, target, channelName + " :End of /NAMES list."));
    }
}