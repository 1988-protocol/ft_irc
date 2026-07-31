#ifndef PRIVMSG_HPP
# define PRIVMSG_HPP

# include "ICommand.hpp"

class Privmsg : public ICommand
{
public:
    Privmsg(Server* server);
    Privmsg(const Privmsg& other);
    Privmsg& operator=(const Privmsg& other);
    ~Privmsg();

    virtual void execute(Client* sender, const std::vector<std::string>& params);
};

#endif