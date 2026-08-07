#include "parser/commands/User.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"

User::User() {}
User::User(const User& other) : ICommand(other) {}
User& User::operator=(const User& other)
{
    (void)other;
    return *this;
}
User::~User() {}

// RFC1459 4.1.3 USER: "<username> <hostname> <servername> :<realname>" — middle 파라미터
// 3개 + trailing 1개가 필요하다. hostname/servername은 RFC상으로도 서버가 신뢰하지 않고
// 직접 판단하는 값이라 실사용하지 않으며, realname을 저장할 필드도 Client.hpp 제안
// 초안(Parser가 실제로 쓰는 필드만 담음) 범위 밖이라 지금은 개수 검증만 하고 버린다.
// username만 저장한다 — 이 축소 범위는 irc/md/parser_message_grammar.md에 기록 대상.
void User::execute(Server& server, Client& client, const Message& msg)
{
    (void)server;
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    if (client.isRegistered())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_ALREADYREGISTRED, target, ":You may not reregister"));
        return;
    }
    // 이 부분은 체크 필요함, 왜냐하면 real_name이 들어가지 않아서 정말 4개가 필요하지 않을수도?
    // 인자 부족(461 ERR_NEEDMOREPARAMS) 시 에러 응답 후 세션을 끊지 않고 리턴하여
    // 클라이언트가 올바른 USER 파라미터를 재전송할 때까지 인증 대기 상태를 유지합니다.
    if (msg.getParams().size() != 3 || !msg.hasTrailing())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NEEDMOREPARAMS, target, "USER :Not enough parameters"));
        return;
    }

    client.setUsername(msg.getParams()[0]);

    if (!client.isRegistered() && client.hasCorrectPassword() && !client.getNickname().empty())
    {
        client.setRegistered(true);
        client.appendToOutBuffer(reply(Numeric::RPL_WELCOME, client.getNickname(),
            ":Welcome to the IRC network, " + client.getNickname()));
    }
}