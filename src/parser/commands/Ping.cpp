// ============================================================================
// [Dependencies - Parser <-> Network Coordination]
// The following Client & Server interfaces are required by Ping command:
//
// Client:
//   - void queueReply(const std::string& line);
//
// Server:
//   - *None*
// ============================================================================

#include "parser/commands/Ping.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Utils.hpp"

Ping::Ping() {}
Ping::Ping(const Ping& other) : ICommand(other) {}
Ping& Ping::operator=(const Ping& other)
{
    (void)other;
    return *this;
}
Ping::~Ping() {}

