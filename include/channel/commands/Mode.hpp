#ifndef MODE_HPP
# define MODE_HPP

#include "ICommand.hpp"

class Mode : public ICommand
{
public:
    Mode(Server* server);
    Mode(const Mode& other);
    Mode& operator=(const Mode& other);
    virtual ~Mode();

    virtual void execute(Client* sender, const std::vector<std::string>& params);

private:
    // 모드 문자별로 처리를 분리해두기
    void handleKey(Client* sender, Channel* channel, bool isAdding, const std::string& arg);
    void handleLimit(Client* sender, Channel* channel, bool isAdding, const std::string& arg);
    void handleOperator(Client* sender, Channel* channel, bool isAdding, const std::string& arg);
};

#endif