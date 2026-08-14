#include "channel/commands/Mode.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"
#include <cstdlib>
#include <sstream>

//RFC 1459 4.2.3.1

namespace 
{
    void appendAppliedMode(std::string& appliedModes, std::string& lastSign, bool isAdding, char modeFlag)
    {
        std::string currentSign;
        if (isAdding)
            currentSign = "+";
        else
            currentSign = "-";

        if (lastSign != currentSign)
        {
            appliedModes += currentSign;
            lastSign = currentSign; // 현재 부호로 기록 갱신
        }
        appliedModes += modeFlag;
    }
}

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

    // 채널 존재 여부 확인
    std::string channelName = params[0];
    Channel* channel = server.getChannel(channelName);
    if (!channel) {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHCHANNEL, target, channelName + " :No such channel"));
        return;
    }

    // 명령 요청 유저가 채널 멤버인지 확인
    if (!channel->isMember(&client)) 
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOTONCHANNEL, target, channelName + " :You're not on that channel"));
        return;
    }

    // 인자가 채널명 하나만 들어온 경우: 단순 모드 상태 조회
    if (params.size() == 1) 
    {
        std::string modeStr = channel->getModeString();
        std::string modeParams = "";

        // +k 모드인 경우 파라미터에 비밀번호 문자열 추가
        if (!channel->getKey().empty())
            modeParams += " " + channel->getKey();

        // +l 모드인 경우 파라미터에 유저 수 추가
        if (channel->getUserLimit() > 0)
        {
            std::ostringstream limitNumber;
            limitNumber << channel->getUserLimit();
            modeParams += " " + limitNumber.str();
        }

        if (modeStr.empty())
            modeStr = "+";

        client.appendToOutBuffer(reply(Numeric::RPL_CHANNELMODEIS, target, channelName + " " + modeStr + modeParams));
        return;
    }

    // 모드 변경 시도 시 방장 권한 확인
    if (!channel->isOperator(&client)) 
    {
        client.appendToOutBuffer(reply(Numeric::ERR_CHANOPRIVSNEEDED, target, channelName + " :You're not channel operator"));
        return;
    }

    std::string modeStr = params[1];
    if (modeStr.size() < 2 || (modeStr[0] != '+' && modeStr[0] != '-')) 
        return;

    bool isAdding = false;
    size_t paramIdx = 2; // 추가 인자가 위치할 인덱스

    std::string appliedModes = ""; // 실제 적용된 모드 기호 모음
    std::string appliedArg = ""; // 추가 인자 저장 변수
    std::string lastSign = "";

    for (size_t i = 0; i < modeStr.size(); ++i)
    {
        char modeFlag = modeStr[i];

        //부호 변경 처리
        if (modeFlag =='+')
        {
            isAdding = true;
            continue;
        }
        if (modeFlag =='-')
        {
            isAdding = false;
            continue;
        }
        // 모드 플래그별 분기 처리
        switch (modeFlag)
        {
            case 'i':
                channel->setInviteOnly(isAdding);
                appendAppliedMode(appliedModes, lastSign, isAdding, 'i');
                break;

            case 't':
                channel->setTopicOpOnly(isAdding);
                appendAppliedMode(appliedModes, lastSign, isAdding, 't');
                break;

            case 'k':
                if (isAdding)
                {
                    if (params.size() <= paramIdx)
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
                        continue;
                    }
                    std::string keyArg = params[paramIdx++];
                    // 이미 비밀번호가 설정되어 있는 경우 변경 차단 (-k 이후 다시 +k 해야함)
                    if (!channel->getKey().empty())
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_KEYSET, target, channelName + " :Channel key already set"));
                        continue;
                    }
                    channel->setKey(keyArg);
                    appendAppliedMode(appliedModes, lastSign, isAdding, 'k');
                    appliedArg += " " + keyArg;
                }
                else
                {
                    channel->removeKey();
                    appendAppliedMode(appliedModes, lastSign, isAdding, 'k');
                }
                break;

            case 'l':
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
                    appendAppliedMode(appliedModes, lastSign, isAdding, 'l');
                    appliedArg += " " + params[paramIdx++];
                }
                else
                {
                    channel->removeUserLimit();
                    appendAppliedMode(appliedModes, lastSign, isAdding, 'l');
                }
                break;

            case 'o':
                {
                    if (params.size() <= paramIdx)
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "MODE :Not enough parameters"));
                        continue;
                    }
                    std::string targetNick = params[paramIdx++];
                    Client* targetClient = server.getClientByNick(targetNick);

                    if (!targetClient)
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHNICK, target, targetNick + " :No such nick/channel"));
                        continue;
                    }
                
                    if (!channel->isMember(targetClient))
                    {
                        client.appendToOutBuffer(reply(Numeric::ERR_USERNOTINCHANNEL, target, targetNick + " " + channelName + " :They aren't on that channel"));
                        continue;
                    }

                    if (isAdding)
                        channel->addOperator(targetClient);
                    else
                        channel->removeOperator(targetClient);
                    appendAppliedMode(appliedModes, lastSign, isAdding, 'o');
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