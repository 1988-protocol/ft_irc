#include "parser/commands/Ping.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"

Ping::Ping() {}
Ping::Ping(const Ping& other) : ICommand(other) {}
Ping& Ping::operator=(const Ping& other)
{
    (void)other;
    return *this;
}
Ping::~Ping() {}

// RFC1459 4.6.2 / RFC2812 3.7.2 PING:
// Parameters: <server1> [<server2>]
// 1. 파라미터가 없는 경우: 409 ERR_NOORIGIN (:No origin specified)
// 2. 파라미터 위치 보존: middle 파라미터목록과 trailing 파라미터를 순서대로 연결하여 index로 접근.
// 3. server2 (두 번째 파라미터) 처리: 2개 이상일 때 server2가 본인 서버 이름과 일치하지 않으면 402 ERR_NOSUCHSERVER
void Ping::execute(Server& server, Client& client, const Message& msg)
{
    (void)server;
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    std::vector<std::string> params = msg.getParams();
    if (msg.hasTrailing())
    {
        params.push_back(msg.getTrailing());
    }

    if (params.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOORIGIN, target, ":No origin specified"));
        return;
    }

    if (params.size() >= 2 && params[1] != getServerName())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NOSUCHSERVER, target, params[1] + " :No such server"));
        return;
    }

    std::string token = params[0];
    client.appendToOutBuffer(":" + getServerName() + " PONG " + getServerName() + " :" + token + "\r\n");
}