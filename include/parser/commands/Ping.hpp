#ifndef PING_HPP
#define PING_HPP

#include "common/ICommand.hpp"

class Ping : public ICommand
{
public:
    Ping();
    Ping(const Ping& other);
    Ping& operator=(const Ping& other);
    ~Ping();

    void execute(Server& server, Client& client, const Message& msg);
};

#endif
