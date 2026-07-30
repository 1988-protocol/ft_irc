// ============================================================================
// [Dependencies - Parser <-> Network Coordination]
// The following Client & Server interfaces are required by Pong command:
//
// Client:
//   - *None*
//
// Server:
//   - *None*
// ============================================================================

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

// RFC1459 4.6.3 PONG: 서버가 보낸 PING에 대한 클라이언트의 응답. 표준상 서버는 별도
// 응답을 보내지 않고 "이 클라이언트가 살아있음"을 기록하기만 하면 된다. 다만 그 기록에
// 쓰일 last-alive 타임스탬프 같은 필드는 Network의 유휴 연결 타임아웃 로직에 속하는
// 범위라 Client.hpp 제안 초안(Parser가 실제로 쓰는 필드만 담음)에는 넣지 않았다 —
// 그 필드가 팀 합의로 추가되면 여기서 갱신하도록 채운다. Phase1에서는 의도적으로
// 아무 동작도 하지 않는다(파라미터 미사용 경고를 피하려고 void 캐스트만 한다).
void Pong::execute(Server& server, Client& client, const Message& msg)
{
    (void)server;
    (void)client;
    (void)msg;
}
