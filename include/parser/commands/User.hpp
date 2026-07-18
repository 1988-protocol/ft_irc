#ifndef USER_HPP
#define USER_HPP

#include "common/ICommand.hpp"

class User : public ICommand
{
public:
    void execute(Server& server, Client& client, const Message& msg);
};

#endif
