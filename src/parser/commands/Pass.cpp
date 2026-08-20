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
// 인자가 없으면 461, 서버 비밀번호와 다르면 464를 응답한다.
// 오류 발생 시 세션을 끊지 않고 에러 응답 후 리턴하여 클라이언트가 올바른 PASS를 재입력할 때까지
// 인증 대기(hasCorrectPassword == false) 상태를 유지한다.
void Pass::execute(Server& server, Client& client, const Message& msg)
{
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    if (client.isRegistered()) // 462: 이미 서버에 들어왔는데 왜 비밀번호를 또 보내는건가요?
    {
        client.appendToOutBuffer(reply(Numeric::ERR_ALREADYREGISTRED, target, ":You may not reregister"));
        return;
    }
    if (msg.getParams().empty()) // 461: 비밀번호 인자가 없어요. PASS 뒤에 아무것도 적지 않았음
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "PASS :Not enough parameters"));
        return;
    }
    if (msg.getParams()[0] != server.getPassword()) // 464: 비밀번호가 틀렸어요. 서버 비밀번호와 다름
    {
        client.appendToOutBuffer(reply(Numeric::ERR_PASSWDMISMATCH, target, ":Password incorrect"));
        return;
    }
    client.setHasCorrectPassword(true);

    // PASS가 NICK/USER 뒤에 올 수도 있으므로 여기서도 등록 완료를 체크
    if (!client.isRegistered() && !client.getNickname().empty() && !client.getUsername().empty())
    {
        client.setRegistered(true);
        client.appendToOutBuffer(reply(Numeric::RPL_WELCOME, client.getNickname(), 
            ":Welcome to the IRC network, " + client.getNickname()));
    }
}

