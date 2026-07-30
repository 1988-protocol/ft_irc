#ifndef NICK_HPP
#define NICK_HPP

#include "common/ICommand.hpp"

class Nick : public ICommand
{
public:
    Nick();
    Nick(const Nick& other);
    Nick& operator=(const Nick& other);
    virtual~Nick();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif
