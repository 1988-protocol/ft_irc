#include "parser/commands/Ping.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"

Ping::Ping() {}
Ping::Ping(const Ping& other) : ICommand(other) {}
Ping& Ping::operator=(const Ping& other)
{
    (void)other;
    return *this;
}
Ping::~Ping() {}

// RFC 1459 Section 4.6.3 (Pong message):
//   Command: PONG
//   Parameters: <daemon> [<daemon2>]
//   "PONG message is a reply to ping message. If parameter <daemon2> is
//    given this message must be forwarded to given daemon. The <daemon>
//    parameter is the name of the daemon who has responded to PING message
//    and generated this message."
//
// [서버 구현 동작 원리 및 규칙]
// 1. PONG은 클라이언트가 서버의 PING에 응답하는 메시지이므로, 서버가 PONG에 대해 다시 응답을 보내서는 안 됨 (루프 방지).
// 2. 단일 서버 환경에서 클라이언트가 보낸 PONG은 생존 확인(Heartbeat) 응답이므로, 별도 타이머가 없는 경우
//    421(ERR_UNKNOWNCOMMAND) 에러를 방지하기 위해 정상 수신(Consume) 후 no-op(빈 동작)으로 처리합니다.
// 이게 올바른 구현이라고 하네요..! 
void Ping::execute(Server& server, Client& client, const Message& msg)
{
    (void)server;
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    std::vector<std::string> params = msg.getParams();
    if (msg.hasTrailing())
    {
        params.push_back(msg.getTrailing());
    }

    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOORIGIN, target, ":No origin specified"));
        return;
    }

    // RFC 1123 Section 2.1 & RFC 1459 Section 2.2:
    // 호스트명 및 IRC 서버 식별자(servername)는 대소문자를 구분하지 않음 (Case-insensitive).
    if (params.size() >= 2 && Utils::toUpper(params[1]) != Utils::toUpper(getServerName()))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHSERVER, target, params[1] + " :No such server"));
        return;
    }

    std::string token = params[0];
    client.appendToOutBuffer(":" + getServerName() + " PONG " + getServerName() + " :" + token + "\r\n");
}