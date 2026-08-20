#ifndef USER_HPP
#define USER_HPP

#include "common/ICommand.hpp"

class User : public ICommand
{
public:
    User();
    User(const User& other);
    User& operator=(const User& other);
    virtual~User();

    virtual void execute(Server& server, Client& client, const Message& msg);
};

#endif
