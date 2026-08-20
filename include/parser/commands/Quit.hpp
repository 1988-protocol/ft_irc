#ifndef QUIT_HPP
#define QUIT_HPP

#include "common/ICommand.hpp"

class Quit : public ICommand
{
public:
    Quit();
    Quit(const Quit& other);
    Quit& operator=(const Quit& other);
    virtual~Quit();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif
