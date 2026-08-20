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

// RFC 1459 Section 4.1.6 (QUIT 명령어)
// Command: QUIT
// Parameters: [<종료 메시지>]
//
// 1. 기본 동작:
//    클라이언트 세션은 QUIT 메시지와 함께 종료됩니다. 서버는 QUIT 메시지를 보낸
//    클라이언트와의 소켓 연결을 반드시 닫아야(close) 합니다.
//    종료 메시지(사유)가 주어진 경우, 기본값 대신 해당 메시지가 동료 유저들에게 전송됩니다.
//
// 2. 넷스플릿(Netsplit) 발생 시:
//    서버 간 연결이 끊어질 때의 종료 메시지는 관련된 두 서버의 이름으로 구성됩니다.
//
// 3. 비정상 종료 시 처리:
//    클라이언트가 QUIT 명령 없이 비정상 종료(EOF 발생 등)된 경우, 서버는 상황의 원인을
//    반영하는 적절한 메시지를 직접 채워서 종료 메시지로 전송해야 합니다.

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

    client.appendToOutBuffer("ERROR :Closing Link: " + reason + "\r\n");
    client.markForDeletion();
}