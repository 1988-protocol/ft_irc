#include "channel/commands/Privmsg.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

Privmsg::Privmsg() : ICommand() {}

Privmsg::~Privmsg() {}

void Privmsg::execute(Server& server, Client& client, Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    const std::string target = client.getNickname();


    // 1. 수신자 미지정 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NORECIPIENT, target, ":No recipient given (PRIVMSG)"));
        return;
    }

    // 메시지 본문 추출 (trailing 우선 가져오기)
    std::string message = "";
    if (msg.hasTrailing())
        message = msg.getTrailing();
    else if (params.size() >= 2)
        message = params[1];

    // 메시지 내용 누락 검사
    if (message.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOTEXTTOSEND, target, ":No text to send"));
        return;
    }

    // 2. 메시지 내용 누락 검사
    std::vector<std::string> targets = Utils::split(params[0], ',');
    for (size_t i = 0; i < targets.size(); ++i)
    {
        std::string targetName = targets[i];
        std::string packet = ":" + client.getNickname() + "!" + client.getUsername() + "@" + client.getHostname()
                       + " PRIVMSG " + targetName + " :" + message + "\r\n";

        // 3. 수신 대상이 채널인 경우 ('#'으로 시작)
        if (!targetName.empty() && targetName[0] == '#')
        {
            Channel* channel = server.getChannel(targetName);
        
            // 채널 존재 여부 검사
            if (!channel)
            {
                client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHNICK, target, targetName + " :No such nick/channel"));
                continue;
            }

            // 보낸 유저가 채널 멤버인지 검사
            if (!channel->isUserInChannel(&client))
            {
                client.appendToOutBuffer(reply(Numeric::ERR_CANNOTSENDTOCHAN, target, targetName + " :Cannot send to channel"));
                continue;
            }

            // 나(sender)를 제외한 채널 내 모든 멤버에게 메시지 전송
            const std::map<Client*, bool>& members = channel->getMembers();
            for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
            {
                if (it->first != &client)
                {
                    it->first->appendToOutBuffer(packet);
                }
            }
        }
        // 4. 수신 대상이 개인 유저인 경우 (1:1 PRIVMSG)
        else
        {
            Client* targetClient = server.getClientByNick(targetName);
        
            // 유저가 존재하지 않는 경우
            if (!targetClient)
            {
                client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHNICK, target, targetName + " :No such nick/channel"));
                continue;
            }

            // 1:1 메시지 전송
            targetClient->appendToOutBuffer(packet);
        }
    }
}