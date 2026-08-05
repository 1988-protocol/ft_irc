#ifndef PRIVMSG_HPP
# define PRIVMSG_HPP

# include "ICommand.hpp"

class Privmsg : public ICommand
{
public:
    Privmsg();
    virtual ~Privmsg();

    virtual void execute(Server& server, Client& client, Message& msg);
};

#endif