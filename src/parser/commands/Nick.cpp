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
