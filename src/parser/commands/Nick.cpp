#include "parser/commands/Nick.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"


Nick::Nick() {}
Nick::Nick(const Nick& other) : ICommand(other) {}
Nick& Nick::operator=(const Nick& other)
{
    (void)other;
    return *this;
}
Nick::~Nick() {}

void Nick::execute(Server& server, Client& client, const Message& msg)
{
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    if (msg.getParams().empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NONICKNAMEGIVEN, target, ":No nickname given"));
        return;
    }

    const std::string& nickname = msg.getParams()[0];

    if (Utils::isSameNickname(nickname, client.getNickname()))
        return; // 이미 쓰고 있는 닉네임과 동일 (RFC1459 대소문자/특수문자 무시) — 에러 아님, 무시

    if (!Utils::isValidNickname(nickname))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_ERRONEUSNICKNAME, target, nickname + " :Erroneous nickname"));
        return;
    }
    if (server.isNicknameInUse(nickname))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NICKNAMEINUSE, target, nickname + " :Nickname is already in use"));
        return;
    }
    // 여기는 Nick을 바꾸고 싶은 상황. 
    // 닉네임의 검증을 새로운 콘테이너가 아닌 기존 콘테이너를 활용하는 방법을 활용하므로 관리로직이 불필요해졌다.
    // 빈 클라이언트에 이름을 등록 중이라면 넘어간다.
    if (!client.getNickname().empty())
        server.releaseNickname(client.getNickname()); // 서버에 기존 닉네임 해제
    server.registerNickname(nickname, client); // 서버에 닉네임 등록
    client.setNickname(nickname); // 클라이언트 닉네임 설정

    // 등록 부분
    // 등록되지 않았고, 클라이언트의 올바를 비밀번호이며, 유저 정보가 있다면.
    if (!client.isRegistered() && client.hasCorrectPassword() && !client.getUsername().empty())
    {
        client.setRegistered(true); // 클라이언트 등록
        client.appendToOutBuffer(reply(Numeric::RPL_WELCOME, nickname, ":Welcome to the IRC network, " + nickname));
        // 환영해요.
    }
}
