#include "Join.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Join::Join(Server* server) : ACommand(server) {}

Join::Join(const Join& other) : ACommand(other) {}

Join& Join::operator=(const Join& other)
{
    if (this != &other)
    {
        ACommand::operator=(other);
    }
    return *this;
}

Join::~Join() {}

void Join::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    if (params.empty())
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 461 " + sender->getNickname() + " JOIN :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];

    // 채널 이름 유효성 검사 ('#'으로 시작해야 함)
    if (channelName.empty() || channelName[0] != '#')
    {
        _server->sendToClient(sender->getFd(), ":ft_irc 403 " + sender->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    // 채널이 존재하지 않으면 새로 생성
    Channel* channel = _server->getChannel(channelName);
    if (!channel)
    {
        channel = new Channel(channelName);
        _server->addChannel(channelName, channel);
        channel->addOperator(sender); // 처음 개설한 유저를 방장으로 지정
    }

    // 이미 참가 중인지 확인
    if (channel->isUserInChannel(sender))
        return;

    // 유저 추가 및 입장 알림 브로드캐스트
    channel->addUser(sender);

    std::string joinMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                            + " JOIN " + channelName + "\r\n";

    const std::vector<Client*>& users = channel->getUsers();
    for (size_t i = 0; i < users.size(); ++i)
    {
        _server->sendToClient(users[i]->getFd(), joinMessage);
    }
}