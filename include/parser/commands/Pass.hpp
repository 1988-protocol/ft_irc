#ifndef PASS_HPP
#define PASS_HPP

#include "common/ICommand.hpp"

class Pass : public ICommand
{
public:
    void execute(Server& server, Client& client, const Message& msg);
};

#endif
