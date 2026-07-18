#ifndef PONG_HPP
#define PONG_HPP

#include "common/ICommand.hpp"

class Pong : public ICommand
{
public:
    void execute(Server& server, Client& client, const Message& msg);
};

#endif
