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

    // TODO(팀 논의 필요): Message 캡슐화(getParamCount / getParam) 합의 시 아래 코드로 대체 가능:
    // if (msg.getParamCount() < 1)
    // {
    //     client.appendToOutBuffer(reply(Numeric::ERR_NONICKNAMEGIVEN, target, ":No nickname given"));
    //     return;
    // }
    // const std::string& nickname = msg.getParam(0);

    if (msg.getParams().empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NONICKNAMEGIVEN, target, ":No nickname given"));
        return;
    }

    const std::string& nickname = msg.getParams()[0];

    if (Utils::isSameNickname(nickname, client.getNickname()))
        return; // 이미 쓰고 있는 닉네임과 동일 (RFC1459 대소문자/특수문자 무시) — 에러 아님, 무시

    // NICK 파라미터 누락, 유효하지 않은 문자열(ERR_ERRONEUSNICKNAME 432), 닉네임 중복(ERR_NICKNAMEINUSE 433)
    // 발생 시 에러 응답 후 세션을 유지(markForDeletion 미호출)하고 닉네임을 변경하지 않아 인증 대기 상태를 유지합니다.
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
    // 닉네임 설정 (m_clients가 single source of truth이므로 client.setNickname만으로 서버 전체에 즉시 반영)
    client.setNickname(nickname);

    // 등록 부분
    // 등록되지 않았고, 클라이언트의 올바를 비밀번호이며, 유저 정보가 있다면.
    if (!client.isRegistered() && client.hasCorrectPassword() && !client.getUsername().empty())
    {
        client.setRegistered(true); // 클라이언트 등록
        client.appendToOutBuffer(reply(Numeric::RPL_WELCOME, nickname, ":Welcome to the IRC network, " + nickname));
        // 환영해요.
    }
}
