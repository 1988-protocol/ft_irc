#include "Mode.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include <cstdlib>

Mode::Mode(Server* server) : ICommand(server) {}

Mode::Mode(const Mode& other) : ICommand(other) {}

Mode& Mode::operator=(const Mode& other) {
    if (this != &other) ICommand::operator=(other);
    return *this;
}

Mode::~Mode() {}

void Mode::execute(Server* server, Client* client, Massage* msg)
{
    if (!server || !client) 
        return;

    // 인자 개수 검사 (461)
    if (msg.params.empty()) 
    {
        m_server->sendToClient(client->getFd(), ":ftm_irc 461 " + client->getNickname() + " MODE :Not enough parameters\r\n");
        return;
    }

    std::string target = msg.params[0];

    // 파라미터 없거나 채널명이 아닐 경우 리턴
    if (target.empty() || target[0] != '#')
        return;

    // 채널 존재 여부 확인 (403)
    Channel* channel = m_server->getChannel(target);
    if (!channel) {
        m_server->sendToClient(client->getFd(), ":ftm_irc 403 " + client->getNickname() + " " + target + " :No such channel\r\n");
        return;
    }

    // 인자가 채널명 하나만 들어온 경우: 단순 모드 상태 조회 (324)
    if (params.size() == 1) {
        std::string modeStr = channel->getModeString();
        if (modeStr.empty())
            modeStr = "+";
        m_server->sendToClient(client->getFd(), ":ftm_irc 324 " + client->getNickname() + " " + target + " " + modeStr + "\r\n");
        return;
    }

    // 명령 요청 유저가 채널 멤버인지 확인 (442)
    if (!channel->isUserInChannel(client)) {
        m_server->sendToClient(client->getFd(), ":ftm_irc 442 " + client->getNickname() + " " + target + " :You're not on that channel\r\n");
        return;
    }

    // 모드 변경 시도 시 방장(Operator) 권한 확인 (482)
    if (!channel->isOperator(client)) {
        m_server->sendToClient(client->getFd(), ":ftm_irc 482 " + client->getNickname() + " " + target + " :You're not channel operator\r\n");
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

        case 'k': // Key
            {
                if (isAdding)
                {
                    if (params.size() <= paramIdx) {
                        m_server->sendToClient(client->getFd(), ":ftm_irc 461 " + client->getNickname() + " MODE :Not enough parameters\r\n");
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

        case 'l': // User Limit
            {
                if (isAdding)
                {
                    if (params.size() <= paramIdx) {
                        m_server->sendToClient(client->getFd(), ":ftm_irc 461 " + client->getNickname() + " MODE :Not enough parameters\r\n");
                        return;
                    }
                    // 리밋 숫자 변환
                    int limit = std::atoi(params[paramIdx].c_str());
                    if (limit <= 0) // 무효한 값이면 리턴
                        return;
                    channel->setUserLimit(limit);
                    appliedArg = " " + params[paramIdx];
                } else {
                    channel->removeUserLimit();
                }
            }
            break;

        case 'o': // Operator 권한 부여/박탈
            {
                if (params.size() <= paramIdx)
                {
                    m_server->sendToClient(client->getFd(), ":ftm_irc 461 " + client->getNickname() + " MODE :Not enough parameters\r\n");
                    return;
                }
                std::string targetNick = params[paramIdx++];
                Client* targetClient = m_server->getClientByNick(targetNick);
                
                if (!targetClient || !channel->isUserInChannel(targetClient))
                {
                    m_server->sendToClient(client->getFd(), ":ftm_irc 441 " + client->getNickname() + " " + targetNick + " " + target + " :They aren't on that channel\r\n");
                    return;
                }

                if (isAdding) channel->addOperator(targetClient);
                else channel->removeOperator(targetClient);
                
                appliedArg = " " + targetNick;
            }
            break;

        default:
            // 알 수 없는 모드 (472)
            m_server->sendToClient(client->getFd(), ":ftm_irc 472 " + client->getNickname() + " " + modeFlag + " :is unknown mode char to me\r\n");
            return;
    }

    // 7. 모드 변경 성공 시 추가 인자까지 포함하여 채널 내 전체 브로드캐스트
    std::string modeNotice = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname()
                           + " MODE " + target + " " + modeStr + appliedArg + "\r\n";

    const std::vector<Client*>& users = channel->getUsers();
    for (sizem_t i = 0; i < users.size(); ++i) {
        m_server->sendToClient(users[i]->getFd(), modeNotice);
    }
}