#include "channel/commands/Privmsg.hpp"
#include "channel/Channel.hpp"
#include "server/Server.hpp"
#include "client/Client.hpp"
#include "parser/Message.hpp"
#include "common/Utils.hpp"
#include "common/Replies.hpp"

//RFC 1459 3.1 / 3.2 / 4.4.1

Privmsg::Privmsg() : ICommand() {}

Privmsg::~Privmsg() {}

void Privmsg::execute(Server& server, Client& client, const Message& msg)
{
    const std::vector<std::string>& params = msg.getParams();
    const std::string target = client.getNickname();

    // 수신자 미지정 검사
    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NORECIPIENT, target, ":No recipient given"));
        return;
    }

    // 메시지 본문 추출
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

    // 수신자목록 파싱
    std::vector<std::string> targets = Utils::split(params[0], ',');

    // 스팸 방지: 수신자 수 제한 검사 (입력된 타겟이 10개 초과 시 에러코드 발송)
    if (targets.size() > 10)
    {
        client.appendToOutBuffer(reply(Numeric::ERR_TOOMANYTARGETS, target, params[0] + " :Too many recipients"));
        return;
    }

    for (size_t i = 0; i < targets.size(); ++i)
    {
        std::string targetName = targets[i];
        // 수신 대상이 채널인 경우
        if (!targetName.empty() && (targetName[0] == '#' || targetName[0] == '&'))
        {
            Channel* channel = server.getChannel(targetName);
        
            // 채널 존재 여부 검사
            if (!channel)
            {
                client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHNICK, target, targetName + " :No such nick/channel"));
                continue;
            }

            // 보낸 유저가 채널 멤버인지 검사
            if (!channel->isMember(&client))
            {
                client.appendToOutBuffer(reply(Numeric::ERR_CANNOTSENDTOCHAN, target, targetName + " :Cannot send to channel"));
                continue;
            }

            // 나를 제외한 채널 내 모든 멤버에게 메시지 전송
            const std::map<Client*, bool>& members = channel->getMembers();
            for (std::map<Client*, bool>::const_iterator it = members.begin(); it != members.end(); ++it)
            {
                if (it->first != &client)
                {
                    it->first->appendToOutBuffer(buildMessage(client, "PRIVMSG", targetName, message));
                }
            }
        }
        // 4. 수신자가 User 개인일 경우
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
            targetClient->appendToOutBuffer(buildMessage(client, "PRIVMSG", targetName, message));
        }
    }
}