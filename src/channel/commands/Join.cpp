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



Join::Join() : ICommand() {}

Join::~Join() {}

void Join::execute(Server& server, Client& client, const Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string target = client.getNickname();

    // 1. params의 인자 개수 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "JOIN :Not enough parameters"));
        return;
    }
    //','기준으로 다중 채널 분리해서 channelName vector 생성
    std::vector<std::string> channelNames = Utils::split(params[0], ',');
    std::vector<std::string> keys;
    //파라미터가 2개 이상이면 두번째 인자부터 키값,키 값도 ','기준으로 분리
    if (params.size() >= 2)
        keys = Utils::split(params[1], ',');
    for (size_t i = 0; i < channelNames.size(); i++)
    {
        std::string channelName = channelNames[i];
        std::string inputKey = "";
        if (keys.size() > i)
            inputKey = keys[i];

        // 채널 이름 유효성 검사
        // RFC 1459 1.3 Channel (채널이름 조건)
        if (!isValidChannelName(channelName))
        {
            client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :Bad channel name"));
            continue;
        }

        // 채널 존재 여부 확인 및 생성/검사
        // 현재는 Server에서 Channel getter, setter, add, remove 함수 있다고 가정
        Channel* channel = server.getChannel(channelName);

        // 채널 갯수 10개 이하 권장
        // 8. 13 Channel membership
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

            // +i 모드 (초대 전용) 검사
            //채널 모드가 invite 이면서, 클라이언트가 invite 되지 않은 상태 확인
            if (channel->isInviteOnly() && !channel->isInvited(&client))
            {
                client.appendToOutBuffer(reply(Numeric::ERR_INVITEONLYCHAN, target, channelName + " :Cannot join channel (+i)"));
                continue;
            }

            // +k 모드 (비밀번호) 검사
            if (channel->isKeyModeActive() && !channel->checkKey(inputKey))
            {
                client.appendToOutBuffer(reply(Numeric::ERR_BADCHANNELKEY, target, channelName + " :Cannot join channel (+k)"));
                continue;
            }

            // +l 모드 (인원 제한) 검사
            if (channel->isFull())
            {
                client.appendToOutBuffer(reply(Numeric::ERR_CHANNELISFULL, target, channelName + " :Cannot join channel (+l)"));
                continue;
            }

            // 최종 유저 추가
            channel->addMember(&client);
        }

        // 4. 초대받아서 들어온 유저라면 초대여부 사용
        if (channel->isInvited(&client))
            channel->removeInvite(&client);

        // 5. 입장 알림 브로드캐스트 (새 유저 포함 채널 내 모든 사람에게 전송)
        const std::map<Client*, bool>& members = channel->getMembers();
        for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            // it->first 객체 자신의 버퍼에 메시지 추가
            it->first->appendToOutBuffer(buildMessage(client, "JOIN", channelName, ""));
        }

        // 6. 입장한 유저(client)에게 Topic 전송
        // Topic 전송
        if (channel->getTopic().empty())
        {
            client.appendToOutBuffer(reply(Numeric::RPL_NOTOPIC, target, channelName + " :No topic is set"));
        }
        else
        {
            client.appendToOutBuffer(reply(Numeric::RPL_TOPIC, target, channelName + " :" + channel->getTopic()));
        }

        //입장한 유저에게 유저목록 전송
        std::string memberList = "";
        
        for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            // 1.3.1 channel operator (유저목록 속 방장은 @표기, 일반 유저는 공백)
            std::string entry = (channel->isOperator(it->first) ? "@" : "") + it->first->getNickname();
            
            // 이번 닉네임을 덧붙였을 때 완성될 닉네임 목록 후보
            std::string candidateList = memberList.empty() ? entry : (memberList + " " + entry);

            // 완성될 353 RPL_NAMREPLY 전체 프레임 생성 후 크기 측정
            std::string testReply = reply(Numeric::RPL_NAMREPLY, target, "= " + channelName + " :" + candidateList);

            //완성된 전체 메시지가 512바이트를 초과하면, 기존까지 모은 memberList를 먼저 전송
            if (!memberList.empty() && (testReply.size() > 510))
            {
                client.appendToOutBuffer(reply(Numeric::RPL_NAMREPLY, target, "= " + channelName + " :" + memberList));
                memberList = entry; // 새 닉네임부터 다시 모으기
            }
            else
            {
                memberList = candidateList;
            }
        }
        //memberList에 잔여 유저 목록 전송
        if (!memberList.empty())
        {
            client.appendToOutBuffer(reply(Numeric::RPL_NAMREPLY, target, "= " + channelName + " :" + memberList));
        }
        // 366 RPL_ENDOFNAMES(목록 전송 완료 신호 - 항상 맨 마지막에 1번만 전송)
        client.appendToOutBuffer(reply(Numeric::RPL_ENDOFNAMES, target, channelName + " :End of /NAMES list."));
    }
}