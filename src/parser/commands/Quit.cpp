#include "parser/commands/Quit.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "channel/Channel.hpp"
#include "common/Utils.hpp"

#include <set>

Quit::Quit() {}
Quit::Quit(const Quit& other) : ICommand(other) {}
Quit& Quit::operator=(const Quit& other)
{
    (void)other;
    return *this;
}
Quit::~Quit() {}

void Quit::execute(Server& server, Client& client, const Message& msg)
{
    std::string reason = "Client Quit";
    if (msg.hasTrailing())
        reason = msg.getTrailing();
    else if (!msg.getParams().empty())
        reason = msg.getParams()[0];

    // 1. 등록된 유저라면 공유 채널 동료들에게 QUIT 브로드캐스트 (본인 제외, 중복 방지)
    if (client.isRegistered() && !client.getNickname().empty())
    {
        std::string quitMsg = buildMessage(client, "QUIT", "", reason);

        std::set<Client*> recipients;
        const std::map<std::string, Channel*>& channels = server.getChannels();
        for (std::map<std::string, Channel*>::const_iterator it = channels.begin(); it != channels.end(); ++it)
        {
            if (it->second && it->second->isMember(&client))
            {
                const std::map<Client*, bool>& members = it->second->getMembers();
                for (std::map<Client*, bool>::const_iterator mIt = members.begin(); mIt != members.end(); ++mIt)
                {
                    if (mIt->first && mIt->first != &client)
                        recipients.insert(mIt->first);
                }
            }
        }

        for (std::set<Client*>::iterator it = recipients.begin(); it != recipients.end(); ++it)
        {
            (*it)->appendToOutBuffer(quitMsg);
        }
    }

    // 2. 본인에게 ERROR 전송 및 종료 플래그 마킹 (자원 정리는 Server::disconnectClient가 전담)
    client.appendToOutBuffer("ERROR :Closing Link: " + reason + "\r\n");
    client.markForDeletion();
}