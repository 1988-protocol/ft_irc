#include "Part.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Part::Part(Server* server) : ACommand(server) {}

Part::Part(const Part& other) : ACommand(other) {}

Part& Part::operator=(const Part& other)
{
    if (this != &other)
    {
        ACommand::operator=(other);
    }
    return *this;
}

Part::~Part() {}

void Part::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    if (params.empty())
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 461 " + sender->getNickname() + " PART :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];

    // 채널 존재 여부 확인
    Channel* channel = _server->getChannel(channelName);
    if (!channel)
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 403 " + sender->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    // 유저가 채널 멤버인지 확인
    if (!channel->isUserInChannel(sender))
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 442 " + sender->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // 퇴장 메시지 전송 및 유저 제거
    std::string partMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                            + " PART " + channelName + "\r\n";

    const std::vector<Client*>& users = channel->getUsers();
    for (size_t i = 0; i < users.size(); ++i)
    {
        _server->sendToClient(users[i]->getFd(), partMessage);
    }

    channel->removeUser(sender);

    // 빈 방 소멸 검사
    if (channel->getUsers().empty())
    {
        _server->removeChannel(channelName);
    }
}