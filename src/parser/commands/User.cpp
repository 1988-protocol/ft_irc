// ============================================================================
// [Dependencies - Parser <-> Network Coordination]
//
// Client:
//   - const std::string& getNickname() const;
//   - bool isRegistered() const;
//   - void queueReply(const std::string& line);
//   - void setUsername(const std::string& username);
//   - bool hasCorrectPassword() const;
//   - void setRegistered(bool value);
//
// Server:
//   - *None*
// ============================================================================

#include "parser/commands/User.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"

User::User() {}
User::User(const User& other) : ICommand(other) {}
User& User::operator=(const User& other)
{
    (void)other;
    return *this;
}
User::~User() {}

