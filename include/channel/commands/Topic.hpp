#ifndef TOPIC_HPP
# define TOPIC_HPP

#include "ICommand.hpp"

class Topic : public ICommand
{
public:
    Topic(Server* server);
    Topic(const Topic& other);
    Topic& operator=(const Topic& other);
    virtual ~Topic();

    virtual void execute(Client* sender, const std::vector<std::string>& params);
};

#endif