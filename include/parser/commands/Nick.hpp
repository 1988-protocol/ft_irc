#ifndef NICK_HPP
#define NICK_HPP

#include "common/ICommand.hpp"

class Nick : public ICommand
{
public:
    void execute(Server& server, Client& client, const Message& msg);
};

#endif
