#ifndef JOIN_HPP
# define JOIN_HPP

# include "ICommand.hpp"

class Join : public ICommand
{
public:
    Join(Server* server);
    Join(const Join& other);
    Join& operator=(const Join& other);
    ~Join();

    virtual void execute(Client* sender, const std::vector<std::string>& params);
};

#endif