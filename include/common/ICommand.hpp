#ifndef ICOMMAND_HPP
#define ICOMMAND_HPP

class Server;
class Client;
class Message;

// 각 커맨드 핸들러(Pass, Nick, User, ... / 추후 Join, Kick 등)가 구현해야 하는 인터페이스.
// Parser가 이 인터페이스만 알고 커맨드를 실행시킨다
// 응답을 반환값(std::string)으로 주지 않고 client.appendToOutBuffer()로 큐잉하는 이유:
// 한 커맨드가 여러 줄을 응답하거나(등록 완료 시 001~004 등) 다른 클라이언트에게도
// 메시지를 보내야 하는 경우(추후 PRIVMSG/JOIN 등)가 있어, 단일 문자열 반환으로는
// 표현할 수 없기 때문이다.
class ICommand
{
public:
    virtual ~ICommand() {}

    // server: NICK 중복 체크 등 서버 전역 상태 조회/변경
    // client: 이 커맨드를 보낸 클라이언트 (등록 상태, 응답 큐)
    // msg: Message::parse()로 이미 분해된 한 줄
    virtual void execute(Server& server, Client& client, const Message& msg) = 0;
};

#endif
