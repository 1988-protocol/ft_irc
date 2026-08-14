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

// RFC 1459 Section 4.1.3 (USER 명령어)
// Command: USER
// Parameters: <username> <hostname> <servername> <realname> - middle 파라미터 3개 + trailing 1개
//
// 1. 기본 목적:
//    USER 메시지는 연결 초기에 신규 유저의 username, hostname, servername, realname을
//    지정하는 데 사용됩니다. 클라이언트로부터 NICK과 USER가 모두 수신되어야만
//    유저 등록(001 RPL_WELCOME)이 완료됩니다.
//
// 2. hostname 및 servername 무시 (보안상 이유):
//    직접 연결된 클라이언트가 보낸 USER 명령어에서 hostname과 servername은
//    보안상의 이유로 IRC 서버가 일반적으로 무시합니다 (서버가 소켓 IP 등으로 직접 판단).
//
// 3. realname (마지막 파라미터):
//    realname은 공백 문자를 포함할 수 있으므로 반드시 마지막 파라미터
//
void User::execute(Server& server, Client& client, const Message& msg)
{
    (void)server;
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

     // Nick과 다르게 등록 후에는 변경할 수 없음. 이건 접속자의 고유 신원이므로.
    if (client.isRegistered())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_ALREADYREGISTRED, target, ":You may not reregister"));
        return;
    }

    size_t totalParams = msg.getParams().size() + (msg.hasTrailing() ? 1 : 0);
    if (totalParams < 4)
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