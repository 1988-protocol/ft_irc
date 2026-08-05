#ifndef JOIN_HPP
# define JOIN_HPP

# include "ICommand.hpp"

class Join : public ICommand
{
public:
    Join();
    virtual ~Join();

    virtual void execute(Server& server, Client& client, Message& msg);
};

#endif