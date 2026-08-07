#include "channel/commands/Mode.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"
#include <cstdlib>
#include <sstream>

Mode::Mode() : ICommand() {}

Mode::~Mode() {}

void Mode::execute(Server& server, Client& client, const Message& msg)
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
        std::string modeParams = "";

        // +k 모드인 경우 파라미터에 비밀번호 문자열 추가
        if (!channel->getKey().empty())
            modeParams += " " + channel->getKey();

        // +l 모드인 경우 파라미터에 유저 수 추가
        if (channel->getUserLimit() > 0)
        {
            std::ostringstream oss;
            oss << channel->getUserLimit();
            modeParams += " " + oss.str();
        }

        if (modeStr.empty())
            modeStr = "+";

        client.appendToOutBuffer(reply(Numeric::RPL_CHANNELMODEIS, target, channelName + " " + modeStr + modeParams));
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

    std::string modeStr = params[1]; //+k, +i, +o 일 수도 있고 +itk 나 +i-t+o 일 수도 있음
    if (modeStr.size() < 2 || (modeStr[0] != '+' && modeStr[0] != '-')) 
        return;

    bool isAdding = (modeStr[0] == '+');
    size_t paramIdx = 2; // 추가 인자가 위치할 인덱스

    std::string appliedModes = ""; // 실제 적용된 모드 기호 모음 (예: "+itk")
    std::string appliedArg = ""; // 브로드캐스트용 추가 인자 저장 변수 (예: "secret user2")

    for (size_t i = 0; i < modeStr.size(); ++i)
    {
        char modeFlag = modeStr[i];

        //부호 변경 처리
        if (modeFlag =='+')
        {
            isAdding = true;
            if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '+')
                appliedModes += '+';
            continue;
        }
        if (modeFlag =='-')
        {
            isAdding = false;
            if (appliedModes.empty() || appliedModes[appliedModes.size() - 1] != '-')
                appliedModes += '-';
            continue;
        }
        // 6. 모드 플래그별 분기 처리
        switch (modeFlag)
        {
            case 'i': // Invite Only
                channel->setInviteOnly(isAdding);
                appliedModes += 'i';
                break;

            case 't': // Topic Op Only
                channel->setTopicOpOnly(isAdding);
                appliedModes += 't';
                break;

            case 'k': // Key (+k password / -k)
                if (isAdding)
                {
                    if (params.size() <= paramIdx)
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
                        continue;
                    }
                    std::string keyArg = params[paramIdx++];
                    channel->setKey(keyArg);
                    appliedModes += 'k';
                    appliedArg += " " + keyArg;
                }
                else
                {
                    channel->removeKey();
                    appliedModes += 'k';
                }
                break;

            case 'l': // User Limit (+l 10 / -l)
                if (isAdding)
                {
                    if (params.size() <= paramIdx) 
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
                        continue;
                    }
                    // 리밋 숫자 변환
                    int limit = std::atoi(params[paramIdx].c_str());
                    if (limit <= 0) // 무효한 값이면 리턴
                    {
                        paramIdx++;
                        continue;
                    }
                    channel->setUserLimit(limit);
                    appliedModes += 'l';
                    appliedArg += " " + params[paramIdx++];
                }
                else
                {
                    channel->removeUserLimit();
                    appliedModes += 'l';
                }
                break;

            case 'o': // Operator 권한 부여/박탈 (+o target / -o target)
                {
                    if (params.size() <= paramIdx)
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
                        continue;
                    }
                    std::string targetNick = params[paramIdx++];
                    Client* targetClient = server.getClientByNick(targetNick);
                
                    if (!targetClient || !channel->isUserInChannel(targetClient))
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_USERNOTINCHANNEL, target, targetNick + " " + channelName + " :They aren't on that channel"));
                        continue;
                    }

                    if (isAdding) channel->addOperator(targetClient); //chanel에 
                    else channel->removeOperator(targetClient);
                    appliedModes += 'o';
                    appliedArg += " " + targetNick;
                }
                break;

            default:
                // 알 수 없는 모드
                client.appendToOutBuffer(reply(Numeric::ERR_UNKNOWNMODE, target, std::string(1, modeFlag)+ " :is unknown mode char to me"));
                break;
        }
    }

    // 실제로 적용된 모드가 1개라도 있는 경우에만 브로드캐스트
    if (!appliedModes.empty() && appliedModes != "+" && appliedModes != "-")
    {
        const std::map<Client*, bool>& members = channel->getMembers();
        for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it) 
        {
            it->first->appendToOutBuffer(buildMessage(client, "MODE", channelName + " " + appliedModes + appliedArg, ""));
        }
    }
}