#ifndef PART_HPP
# define PART_HPP

# include "ICommand.hpp"

class Part : public ICommand
{
public:
    Part();
    virtual ~Part();

    virtual void execute(Server& server, Client& client, Message& msg);
};

#endif