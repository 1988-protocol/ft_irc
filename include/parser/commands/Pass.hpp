#ifndef PASS_HPP
#define PASS_HPP

#include "common/ICommand.hpp"

class Pass : public ICommand
{
public:
    Pass();
    Pass(const Pass& other);
    Pass& operator=(const Pass& other);
    virtual~Pass();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif
