#ifndef PONG_HPP
#define PONG_HPP

#include "common/ICommand.hpp"

class Pong : public ICommand
{
public:
    Pong();
    Pong(const Pong& other);
    Pong& operator=(const Pong& other);
    ~Pong();

    void execute(Server& server, Client& client, const Message& msg);
};

#endif
