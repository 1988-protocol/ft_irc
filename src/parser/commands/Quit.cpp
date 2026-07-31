// ============================================================================
// [Dependencies - Parser <-> Network Coordination]
// The following Client & Server interfaces are required by Quit command:
//
// Client:
//   - const std::string& getNickname() const;
//   - void queueReply(const std::string& line);
//
// Server:
//   - void releaseNickname(const std::string& nickname);
// ============================================================================

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
