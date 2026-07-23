#ifndef INVITE_HPP
# define INVITE_HPP

# include "ACommand.hpp"

class Invite : public ACommand
{
public:
    Invite(Server* server);
    Invite(const Invite& other);
    Invite& operator=(const Invite& other);
    ~Invite();

    virtual void execute(Client* sender, const std::vector<std::string>& params);
};

#endif