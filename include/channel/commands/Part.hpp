#ifndef PART_HPP
# define PART_HPP

# include "common/ICommand.hpp"

class Part : public ICommand
{
public:
    Part();
    virtual ~Part();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif