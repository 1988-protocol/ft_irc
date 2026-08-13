#ifndef PARSER_HPP
#define PARSER_HPP

#include <map>
#include <string>

class Server;
class Client;
class ICommand;

// Network가 넘겨준 한 줄을
// Message::parse()로 분해한 뒤, 등록된 ICommand 핸들러에 실행을 위임한다.
class Parser
{
public:
    // 생성자에서 PASS/NICK/USER/PING/PONG/QUIT 및 채널/공용 명령어 핸들러를 생성해 등록한다.
    Parser();

    // 생성자에서 new로 소유한 ICommand*들을 여기서 delete한다.
    ~Parser();

    // Network -> Parser 경계: 누적 버퍼에서 \r\n 단위로
    // 잘라낸 한 줄(rawLine)을 받아 파싱부터 커맨드 실행까지 이 한 번의 호출로 처리한다.
    void process(Server& server, Client& client, const std::string& rawLine);

private:
    std::map<std::string, ICommand*> m_commands;

    void registerCommand(const std::string& name, ICommand* handler);

    // 미등록(PASS/NICK/USER 시퀀스 미완료) 상태에서도 허용되는 커맨드인지 판단한다.
    // 등록 시퀀스 자체(PASS/NICK/USER)와 연결 유지/종료(PING/PONG/QUIT)는 예외로 둔다.
    bool isAllowedBeforeRegistration(const std::string& command) const;

    // m_commands가 소유한 포인터를 얕은 복사하면 이중 delete로 이어지므로 복사를 금지한다.
    // C++98 방식: private으로 선언만 하고 정의하지 않는다.
    // 이렇게 private에 놓음으로써 복사나 대입을 할 수 없게 함.
    Parser(const Parser&);
    Parser& operator=(const Parser&);
};

#endif
