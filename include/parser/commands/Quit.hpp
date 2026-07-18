#ifndef QUIT_HPP
#define QUIT_HPP

#include "common/ICommand.hpp"

class Quit : public ICommand
{
public:
    void execute(Server& server, Client& client, const Message& msg);
};

#endif
