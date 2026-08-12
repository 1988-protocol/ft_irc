#include "parser/commands/Pong.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"

Pong::Pong() {}
Pong::Pong(const Pong& other) : ICommand(other) {}
Pong& Pong::operator=(const Pong& other)
{
    (void)other;
    return *this;
}
Pong::~Pong() {}

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
void Pong::execute(Server& server, Client& client, const Message& msg)
{
    (void)server;
    (void)client;
    (void)msg;
}