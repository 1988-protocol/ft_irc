#ifndef TOPIC_HPP
# define TOPIC_HPP

#include "common/ICommand.hpp"

class Topic : public ICommand
{
public:
    Topic();
    virtual ~Topic();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif