#ifndef JOIN_HPP
# define JOIN_HPP

# include "common/ICommand.hpp"

class Join : public ICommand
{
public:
    Join();
    virtual ~Join();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif