#ifndef INVITE_HPP
# define INVITE_HPP

# include "common/ICommand.hpp"

class Invite : public ICommand
{
public:
    Invite();
    virtual ~Invite();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif