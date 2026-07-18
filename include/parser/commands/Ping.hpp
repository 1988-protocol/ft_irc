#ifndef PING_HPP
#define PING_HPP

#include "common/ICommand.hpp"

class Ping : public ICommand
{
public:
    void execute(Server& server, Client& client, const Message& msg);
};

#endif
