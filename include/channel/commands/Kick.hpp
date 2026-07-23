#ifndef KICK_HPP
# define KICK_HPP

# include "Acommand.hpp"

class Kick : public Acommand
{
public:
    Kick(Server* server);
    Kick(const Kick& other);
    Kick& operator=(const Kick& other);
    ~Kick();

    virtual void execute(Client* sender, const std::vector<std::string>& params);
};

#endif