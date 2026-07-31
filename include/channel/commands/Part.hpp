#ifndef PART_HPP
# define PART_HPP

# include "ICommand.hpp"

class Part : public ICommand
{
public:
    Part(Server* server);
    Part(const Part& other);
    Part& operator=(const Part& other);
    ~Part();

    virtual void execute(Client* sender, const std::vector<std::string>& params);
};

#endif