#include "parser/commands/Pong.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"

Pong::Pong() {}
Pong::Pong(const Pong& other) : ICommand(other) {}
Pong& Pong::operator=(const Pong& other)
{
    (void)other;
    return *this;
}
Pong::~Pong() {}

// 서버가 PONG 메시지를 받았을 때 클라이언트에게 추가 응답을 전송하면 안됨.
void Pong::execute(Server& server, Client& client, const Message& msg)
{
    (void)server;
    (void)client;
    (void)msg;
}