#include "channel/commands/Join.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

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
        // 채널 이름 비어있거나 맨 첫 글자가 #이 나리면 403에러
        if (channelName.empty() || channelName[0] != '#')
        {
            client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
            continue;
        }

        // 채널 존재 여부 확인 및 생성/검사
        // 현재는 Server에서 Channel getter, setter, add, remove 함수 있다고 가정
        Channel* channel = server.getChannel(channelName);
        if (!channel)
        {
            // 처음 생성될 때는 채널 객체 생성 후 유저 추가 및 방장 지정
            channel = new Channel(channelName);
            server.addChannel(channelName, channel);
            channel->addUser(&client);
            channel->addOperator(&client);
        }
        else
        {
            // 이미 참가 중인 유저는 중복 진입 방지
            if (channel->isUserInChannel(&client))
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
            channel->addUser(&client);
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
        std::string userList = "";
        // 안전한 닉네임 전송 길이 (최대 512인데 앞에 붙는 서버이름, 닉네임, 채널명 헤더)
        const size_t MAX_USER_LIST_LEN = 400;
        for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
        {
            std::string entry = (channel->isOperator(it->first) ? "@" : "") + it->first->getNickname();
            
            // 새 닉네임을 붙였을 때 400자를 초과하면, 지금까지 쌓인 목록을 먼저 353으로 전송
            if (!userList.empty() && (userList.size() + entry.size() + 1 > MAX_USER_LIST_LEN))
            {
                client.appendToOutBuffer(reply(Numeric::RPL_NAMREPLY, target, "= " + channelName + " :" + userList));
                userList = "";
            }
            if (!userList.empty())
                userList += " ";
            userList += entry;
        }
        //userList에 남은 내용 전송
        if (!userList.empty())
        {
            client.appendToOutBuffer(reply(Numeric::RPL_NAMREPLY, target, "= " + channelName + " :" + userList));
        }
        // 366 RPL_ENDOFNAMES(목록 전송 완료 신호 - 항상 맨 마지막에 1번만 전송)
        client.appendToOutBuffer(reply(Numeric::RPL_ENDOFNAMES, target, channelName + " :End of /NAMES list."));
    }
}