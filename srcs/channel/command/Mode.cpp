#include "channel/commands/Mode.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"
#include <cstdlib>

Mode::Mode() : ICommand() {}

Mode::~Mode() {}

void Mode::execute(Server& server, Client& client, Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    std::string target = client.getNickname();
    
    // 인자 개수 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
        return;
    }

    // 명령어: MODE / 파라미터: <channel> {[+|-]|o|p|s|i|t|n|b|v} [<limit>] [<user(닉네임)>] [<ban mask>]
    std::string channelName = params[0];

    // 파라미터 없거나 채널명이 아닐 경우 리턴
    if (channelName.empty() || channelName[0] != '#')
        return;

    // 채널 존재 여부 확인
    Channel* channel = server.getChannel(channelName);
    if (!channel) {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
        return;
    }

    // 인자가 채널명 하나만 들어온 경우: 단순 모드 상태 조회
    if (params.size() == 1) {
        std::string modeStr = channel->getModeString();
        if (modeStr.empty())
            modeStr = "+";
        client.appendToOutBuffer(reply(Numeric::RPL_CHANNELMODEIS, target, channelName + " :" + modeStr));
        return;
    }

    // 명령 요청 유저가 채널 멤버인지 확인
    if (!channel->isUserInChannel(&client)) {
        client.appendToOutBuffer(reply(Numeric::ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
        return;
    }

    // 모드 변경 시도 시 방장(Operator) 권한 확인
    if (!channel->isOperator(&client)) {
        client.appendToOutBuffer(reply(Numeric::ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
        return;
    }

    std::string modeStr = params[1]; //+k, +i, +o
    if (modeStr.size() < 2 || (modeStr[0] != '+' && modeStr[0] != '-')) 
        return;

    bool isAdding = (modeStr[0] == '+');
    char modeFlag = modeStr[1]; // +인자 빼고 어떤 모드인지 확인하기 위한 알파벳 체크용
    int paramIdx = 2; // 추가 인자가 위치할 인덱스

    std::string appliedArg = ""; // 브로드캐스트용 추가 인자 저장 변수

    // 6. 모드 플래그별 분기 처리
    switch (modeFlag)
    {
        case 'i': // Invite Only
            channel->setInviteOnly(isAdding);
            break;

        case 't': // Topic Op Only
            channel->setTopicOpOnly(isAdding);
            break;

        case 'k': // Key (+k password / -k)
            {
                if (isAdding)
                {
                    if (params.size() <= paramIdx) {
                        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
                        return;
                    }
                    std::string keyArg = params[paramIdx++];
                    channel->setKey(keyArg);
                    appliedArg = " " + keyArg;
                } else {
                    channel->removeKey();
                }
            }
            break;

        case 'l': // User Limit (+l 10 / -l)
            {
                if (isAdding)
                {
                    if (params.size() <= paramIdx) {
                        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
                        return;
                    }
                    // 리밋 숫자 변환
                    int limit = std::atoi(params[paramIdx].c_str());
                    if (limit <= 0) // 무효한 값이면 리턴
                        return;
                    channel->setUserLimit(limit);
                    appliedArg = " " + params[paramIdx++];
                } else {
                    channel->removeUserLimit();
                }
            }
            break;

        case 'o': // Operator 권한 부여/박탈 (+o target / -o target)
            {
                if (params.size() <= paramIdx)
                {
                    client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
                    return;
                }
                std::string targetNick = params[paramIdx++];
                Client* targetClient = server.getClientByNick(targetNick);
                
                if (!targetClient || !channel->isUserInChannel(targetClient))
                {
                    client.appendToOutBuffer(reply(Numeric::ERR_USERNOTINCHANNEL, target, targetNick + " " + channelName + " :They aren't on that channel"));
                    return;
                }

                if (isAdding) channel->addOperator(targetClient); //chanel에 
                else channel->removeOperator(targetClient);
                
                appliedArg = " " + targetNick;
            }
            break;

        default:
            // 알 수 없는 모드
            client.appendToOutBuffer(reply(Numeric::ERR_UNKNOWNMODE, target, std::string(1, modeFlag)+ " :is unknown mode char to me"));
            return;
    }

    // 7. 모드 변경 성공 시 추가 인자까지 포함하여 채널 내 전체 브로드캐스트
    const std::map<Client*, bool>& members = channel->getMembers();
    for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it) 
    {
        it->first->appendToOutBuffer(buildMessage(client, "MODE", channelName + " " + modeStr + appliedArg));
    }
}