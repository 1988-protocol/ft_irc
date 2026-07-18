#include "parser/commands/Nick.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"

namespace
{
    // RFC2812 2.3.1 nickname 문자 집합을 간략화한 버전: 첫 글자는 알파벳이거나 special
    // 문자, 이후 글자는 알파벳/숫자/special. 42 과제 범위에서는 전체 RFC 규격을 다
    // 구현할 필요가 없어 최소 구현으로 두었다(길이 제한도 의도적으로 미검사) —
    // 이 축소 범위는 irc/md/parser_message_grammar.md에 기록한다.
    bool isValidNicknameChar(char c, bool isFirst)
    {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            return true;
        if (!isFirst && c >= '0' && c <= '9')
            return true;
        static const std::string specials = "-[]\\`_^{|}";
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
// 이전 닉네임을 해제한다. 근거: irc/md/parser_message_grammar.md 4.6.
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
