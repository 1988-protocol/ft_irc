#ifndef KICK_HPP
# define KICK_HPP

# include "ICommand.hpp"

class Kick : public ICommand
{
public:
    Kick(Server* server);
    Kick(const Kick& other);
    Kick& operator=(const Kick& other);
    ~Kick();

    virtual void execute(Client* sender, const std::vector<std::string>& params);
};

#endif