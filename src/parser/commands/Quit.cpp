#include "parser/commands/Quit.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"

// RFC1459 4.1.6 QUIT: 클라이언트가 연결 종료를 요청한다. 실제 fd close는 Network의
// poll 루프 몫이라 Parser가 직접 연결을 끊을 수는 없다 — outbox에 ERROR 라인을 큐잉해
// "이 클라이언트는 종료 대상"이라는 신호만 남긴다. Network가 이 큐를 flush한 뒤 fd를
// 닫는 구체적 트리거 방식(예: Client에 별도 종료 플래그를 둘지, ERROR 전송 자체를
// 신호로 볼지)은 Client.hpp가 아직 제안 단계라 확정하지 못했다 — 16일 회의 안건으로
// irc/md/parser_message_grammar.md에 기록해 둔다.
//
// 리뷰에서 발견된 버그 수정(2026-07-18): 닉네임은 실제 fd가 닫히기 전, QUIT을 처리하는
// 이 시점에 즉시 반환해야 한다 — 그래야 다른 클라이언트가 곧바로 그 닉네임을 다시 쓸 수
// 있다(실제 IRC 서버 동작과 동일). fd close까지 기다리면 releaseNickname 호출 지점이
// Network 쪽으로 넘어가야 하는데, 아직 그 신호 방식이 미확정이라(위 문단) 여기서 먼저
// 처리한다. 
void Quit::execute(Server& server, Client& client, const Message& msg)
{
    if (!client.getNickname().empty())
        server.releaseNickname(client.getNickname());

    std::string reason = msg.hasTrailing() ? msg.getTrailing() : "Leaving";
    client.queueReply("ERROR :Closing Link: " + reason + "\r\n");
    //client의 close하는 함수 호출해야 함.
}
