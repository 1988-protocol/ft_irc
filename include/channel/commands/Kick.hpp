#ifndef KICK_HPP
# define KICK_HPP

# include "ICommand.hpp"

class Kick : public ICommand
{
public:
    Kick();
    virtual ~Kick();

    virtual void execute(Server& server, Client& client, Message& msg);
};

#endif