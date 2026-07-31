#ifndef INVITE_HPP
# define INVITE_HPP

# include "ICommand.hpp"

class Invite : public ICommand
{
public:
    Invite(Server* server);
    Invite(const Invite& other);
    Invite& operator=(const Invite& other);
    ~Invite();

    virtual void execute(Client* sender, const std::vector<std::string>& params);
};

#endif