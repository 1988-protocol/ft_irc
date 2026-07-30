// ============================================================================
// [Dependencies - Parser <-> Network Coordination]
// The following Client & Server interfaces are required by Ping command:
//
// Client:
//   - void queueReply(const std::string& line);
//
// Server:
//   - *None*
// ============================================================================

#include "parser/commands/Ping.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Utils.hpp"

Ping::Ping() {}
Ping::Ping(const Ping& other) : ICommand(other) {}
Ping& Ping::operator=(const Ping& other)
{
    (void)other;
    return *this;
}
Ping::~Ping() {}

// RFC1459 4.6.2 PING: 클라이언트가 보낸 토큰을 그대로 PONG으로 돌려줘 연결이 살아있음을
// 확인시켜준다. numeric reply가 아니라 별도 커맨드 응답이라 reply() 헬퍼를 쓰지 않고
// 직접 라인을 만든다. 서버가 유휴 클라이언트에게 먼저 PING을 보내는 능동적 헬스체크는
// poll() 타이머를 다루는 Network의 몫이라 여기서는 다루지 않는다.
void Ping::execute(Server& server, Client& client, const Message& msg)
{
    (void)server;
    std::string token = msg.hasTrailing() ? msg.getTrailing()
        : (msg.getParams().empty() ? "" : msg.getParams()[0]);

    client.queueReply(":" + getServerName() + " PONG " + getServerName() + " :" + token + "\r\n");
}
