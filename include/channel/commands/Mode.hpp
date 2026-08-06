#ifndef MODE_HPP
# define MODE_HPP

#include "common/ICommand.hpp"

class Mode : public ICommand
{
public:
    Mode();
    virtual ~Mode();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif