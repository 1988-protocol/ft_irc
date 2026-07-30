#include "parser/commands/Pass.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"

Pass::Pass() {}
Pass::Pass(const Pass& other) : ICommand(other) {}
Pass& Pass::operator=(const Pass& other)
{
    (void)other;
    return *this;
}
Pass::~Pass() {}

// RFC1459 4.1.1 PASS: 등록 시퀀스의 첫 단계. 이미 등록된 클라이언트가 다시 보내면 462,
// 인자가 없으면 461, 서버 비밀번호와 다르면 464를 응답한다. 성공해도 여기서는 "비밀번호
// 확인됨" 상태만 남기고, 실제 등록 완료(001 전송) 판정은 Nick/User가 맡는다 — PASS/NICK/USER
// 순서를 엄격히 강제하지 않고 셋 다 채워지면 등록 완료로 보는 관대한 해석을 택했다(실제
// 클라이언트들이 순서를 지키지 않는 경우가 흔함). 
void Pass::execute(Server& server, Client& client, const Message& msg)
{
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    if (client.isRegistered())
    {
        client.queueReply(reply(Numeric::ERR_ALREADYREGISTRED, target, ":You may not reregister"));
        return;
    }
    if (msg.getParams().empty())
    {
        client.queueReply(reply(Numeric::ERR_NEEDMOREPARAMS, target, "PASS :Not enough parameters"));
        return;
    }
    if (msg.getParams()[0] != server.getPassword())
    {
        client.queueReply(reply(Numeric::ERR_PASSWDMISMATCH, target, ":Password incorrect"));
        return;
    }
    client.setHasCorrectPassword(true);
}
