// ============================================================================
// [Dependencies - Parser <-> Network Coordination]
// 클라이언트와 서버가 닉네임을 처리하기 위해 필요한 인터페이스
//
// Client:
//   필요한 멤버 변수:     
//   - bool m_registered;
//   - bool m_hasCorrectPassword;
//   - std::string m_nickname;
//   - std::string m_username;

//   필요한 게터: getNickname, getUsername, isRegistered, hasCorrectPassword
//   필요한 세터: setNickname, setRegistered
//   필요한 멤버 함수: appendToOutBuffer(IRC프로토콜의 맞는 메시지를 송신 버퍼에 저장하는 함수) // 기존 있음
//   - const std::string& getNickname() const;
//   - const std::string& getUsername() const;
//   - bool hasCorrectPassword() const;
//   - bool isRegistered() const;

//   - void setNickname(const std::string& nickname);
//   - void setRegistered(bool value);

//   - void appendToOutBuffer(const std::string& line); 
//
// Server:
//   필요한 멤버 함수(메서드)
//   - bool isNicknameInUse(const std::string& nickname); 질의
//   - void releaseNickname(const std::string& nickname); 상태 변경
//   - void registerNickname(const std::string& nickname, Client& client); 상태 변경
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
    // nick이 아직 정해지지 않은 상태에서는 무엇을 넣어야 하는지 나와있지 않음
    // 그렇다고 공백을 할 수 없어서
    // 시카고 대학교(University of Chicago)의 IRC 프로젝트(chirc) 명세서를 참고해서 규격을 맞춤

    if (msg.getParams().empty())
    {
        // (예)appendToOutBuffer는 Reply라는 메시지 포맷터에 의해 완벽히 완성된 문자열(\r\n이 포함된 string)을 매개변수로 받아서
        // client의 송신 버퍼에 데이터를 추가해야 합니다. 
        // 일종의 perror 의 역할을 하지만,  네트워크 멀티플렉싱(poll/select) 서버에서 소켓을 대상으로 send() 시스템 콜을 즉시 호출하면 안 되기 때문에 
        // 버퍼에 담아두었다가 한 번에 전송하는 구조를 사용합니다.
        client.appendToOutBuffer(reply(Numeric::ERR_NONICKNAMEGIVEN, target, ":No nickname given"));
        return;
    }

    const std::string& nickname = msg.getParams()[0];

    if (nickname == client.getNickname())
        return; // 이미 쓰고 있는 닉네임과 동일 — 에러 아님, 아무 효과 없이 무시

    if (!isValidNickname(nickname))
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
    /*
    // 빈 클라이언트에 이름을 등록 중이라면 넘어간다.
    if (!client.getNickname().empty())
        server.releaseNickname(client.getNickname()); // 서버에 기존 닉네임 해제
    server.registerNickname(nickname, client); // 서버에 닉네임 등록
    */
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
