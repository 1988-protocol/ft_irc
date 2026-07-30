// ============================================================================
// [Dependencies - Parser <-> Network Coordination]
// The following Client & Server interfaces are required by Nick command:
//
// Client:
//   - const std::string& getNickname() const;
//   - void queueReply(const std::string& line);
//   - void setNickname(const std::string& nickname);
//   - bool isRegistered() const;
//   - void setRegistered(bool value);
//   - bool hasCorrectPassword() const;
//   - const std::string& getUsername() const;
//
// Server:
//   - bool isNicknameInUse(const std::string& nickname);
//   - void releaseNickname(const std::string& nickname);
//   - void registerNickname(const std::string& nickname, Client& client);
// ============================================================================

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



namespace
{
    // RFC1459 2.3.1 nickname 규격: 첫 글자는 알파벳, 이후 글자는 알파벳/숫자/special(-[]\`^{}).
    // 최대 길이는 9자 제한(RFC1459 1.2절).
    bool isValidNicknameChar(char c, bool isFirst)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            return true;
        if (!isFirst && c >= '0' && c <= '9')
            return true;
        static const std::string specials = "-[]\\`_^{|}";
        // static을 선언함으로써 함수가 호출될 때마다 메모리를 할당하지 않고, 한번만 생성해서
        // 이후 호출부터는 재사용
        return specials.find(c) != std::string::npos;
    }

    bool isValidNickname(const std::string& nickname)
    {
        if (nickname.empty())
            return false;
        for (std::string::size_type i = 0; i < nickname.size(); ++i)
        {
            if (!isValidNicknameChar(nickname[i], i == 0))
                return false;
        }
        return true;
    }
}

// RFC1459 4.1.2 NICK: 431(인자 없음) -> 432(형식 위반) -> 433(중복) 순으로 검사한다.
// 성공 시 서버의 닉네임 레지스트리에 등록하고, PASS/USER까지 이미 끝났다면(순서 무관,
// Pass.cpp 주석 참고) 이 시점에 001 RPL_WELCOME을 보내고 등록을 완료 처리한다.
//
// 리뷰에서 발견된 버그 수정(2026-07-18): 자기 자신이 이미 쓰고 있는 닉네임을 그대로
// 재전송하면 isNicknameInUse()가 자신의 map 엔트리를 찾아 false positive로 433을
// 반환했다 — 동일 닉네임 재전송은 사전에 no-op으로 분리해 회피한다. 또한 닉네임을
// 실제로 변경할 때 이전 엔트리를 releaseNickname()으로 지우지 않으면 예전 닉네임이
// map에 영구히 남아 다른 클라이언트가 재사용할 수 없었다 — registerNickname() 전에
// 이전 닉네임을 해제한다.
void Nick::execute(Server& server, Client& client, const Message& msg)
{
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    if (msg.getParams().empty())
    {
        client.queueReply(reply(Numeric::ERR_NONICKNAMEGIVEN, target, ":No nickname given"));
        return;
    }

    const std::string& nickname = msg.getParams()[0];

    if (nickname == client.getNickname())
        return; // 이미 쓰고 있는 닉네임과 동일 — 에러 아님, 아무 효과 없이 무시

    if (!isValidNickname(nickname))
    {
        client.queueReply(reply(Numeric::ERR_ERRONEUSNICKNAME, target, nickname + " :Erroneous nickname"));
        return;
    }
    if (server.isNicknameInUse(nickname))
    {
        client.queueReply(reply(Numeric::ERR_NICKNAMEINUSE, target, nickname + " :Nickname is already in use"));
        return;
    }

    if (!client.getNickname().empty())
        server.releaseNickname(client.getNickname());
    server.registerNickname(nickname, client);
    client.setNickname(nickname);

    if (!client.isRegistered() && client.hasCorrectPassword() && !client.getUsername().empty())
    {
        client.setRegistered(true);
        client.queueReply(reply(Numeric::RPL_WELCOME, nickname, ":Welcome to the IRC network, " + nickname));
    }
}
