#include "parser/commands/Quit.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"

Quit::Quit() {}
Quit::Quit(const Quit& other) : ICommand(other) {}
Quit& Quit::operator=(const Quit& other)
{
    (void)other;
    return *this;
}
Quit::~Quit() {}

void Quit::execute(Server& server, Client& client, const Message& msg)
{
    if (!client.getNickname().empty())
        server.releaseNickname(client.getNickname());

    std::string reason = msg.hasTrailing() ? msg.getTrailing() : "Leaving";
    client.appendToOutBuffer("ERROR :Closing Link: " + reason + "\r\n");
    client.markForDeletion();
}