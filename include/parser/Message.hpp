#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <vector>

// 레이어 경계(pre_plan.md Phase0 합의):
//   Network -> Parser: 누적 버퍼에서 \r\n 단위로 이미 잘라낸 한 줄(std::string)
//   Parser -> Channel/Cmd: 이 Message 객체를 그대로 넘긴다
//
// Rule of Zero: std::string/std::vector만 멤버로 가지므로 컴파일러가 만드는 기본
// 복사생성자/대입연산자/소멸자로 충분하다 — 직접 정의하지 않는다.
class Message
{
public:
    Message();

    // rawLine(예: ":nick!user@host PRIVMSG #chan :hello world")을 RFC1459 2.3.1
    // 문법에 따라 prefix/command/params/trailing으로 분해한다. 구현은 Message.cpp,
    // Phase1에서 Parser가 채운다.
    static Message parse(const std::string& rawLine);

    const std::string& getPrefix() const;
    const std::string& getCommand() const;
    const std::vector<std::string>& getParams() const;
    const std::string& getTrailing() const;

    // trailing이 "존재하되 빈 문자열"인 경우(예: "PRIVMSG #chan :")와 "애초에 trailing이
    // 없는 경우"(예: "NICK bob")를 구분해야 하므로 getTrailing()의 빈 문자열만으로는
    // 판별할 수 없다 — 이 플래그로 명시적으로 구분한다.
    bool hasTrailing() const;

    void setPrefix(const std::string& prefix);
    void setCommand(const std::string& command);
    void addParam(const std::string& param);
    void setTrailing(const std::string& trailing);

private:
    std::string m_prefix;
    std::string m_command;
    std::vector<std::string> m_params;
    std::string m_trailing;
    bool m_hasTrailing;
};

#endif
