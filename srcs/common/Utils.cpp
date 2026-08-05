// 1. 상단 #include 구문 맨 아래에 1줄 추가
#include "client/Client.hpp"

// 2. 기존 reply(...) 함수 본문 끝나는 지점 아래에 함수 1개 구현 추가
std::string buildMessage(const Client& client, const std::string& cmd, const std::string& target, const std::string& msg)
{
    std::string line = ":" + client.getNickname() + "!" + client.getUsername() + "@" + client.getHostname();
    line += " " + cmd;
    if (!target.empty())
        line += " " + target;
    if (!msg.empty())
        line += " :" + msg;
    line += "\r\n";
    return line;
}
