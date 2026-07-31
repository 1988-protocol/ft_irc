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
