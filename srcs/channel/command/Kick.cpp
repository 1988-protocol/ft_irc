#include "Kick.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Kick::Kick(Server* server) : ACommand(server) {}

Kick::Kick(const Kick& other) : ACommand(other) {}

Kick& Kick::operator=(const Kick& other)
{
    if (this != &other)
    {
        ACommand::operator=(other);
    }
    return *this;
}

Kick::~Kick() {}

void Kick::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

        if (params.size() < 2)
        {
            _server->sendToClient(sender->getFd(), ":ft_irc 461 " + sender->getNickname() + " KICK :Not enough parameters\r\n");
            return ;
        }

        std::string channelName = params[0];
        std::string targetNick = params[1];
        std::string reason = (params.size() >= 3) ? params[2] : "No reason specified";

        // 채널 존재 여부 확인
        //Server에 getChannel 헬퍼 함수 작성
        Channel* channel = _server->getChannel(channelName);
        if (!channel)
        {
            //ERR_NOSUCHCHANNEL (403)
            _server->sendToClient(sender->getFd(), ":ft_irc 403 " + sender->getNickname() + " " + channelName + " :No such channel\r\n");
            return ;
        }

        // 명령 내린 sender가 채널 멤버인지 확인
        if (!channel->isUserInChannel(sender))
        {
            //ERR_NOSUCHCHANNEL (442)
            _server->sendToClient(sender->getFd(), ":ft_irc 442 " + sender->getNickname() + " " + channelName + " :You're not in that channel\r\n");
            return ;
        }

        // sender이 operator인지 확인
        if (!channel->isOperator(sender))
        {
            //ERR_NOSUCHCHANNEL (482)
            _server->sendToClient(sender->getFd(), ":ft_irc 482 " + sender->getNickname() + " " + channelName + " :You're not in that channel\r\n");
            return ;
        }

        // 강퇴 대상(target) 찾기 채널 참가 여부 확인
        Client* target = _server->getClientByNick(targetNick);
        if (!target || !channel->isUserInChannel(target))
        {
            //ERR_NOSUCHCHANNEL (441)
            _server->sendToClient(sender->getFd(), ":ft_irc 441 " + sender->getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
            return ;
        }

        //강퇴 메시지 전송하고 실행하기
        std::string kickMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                                + " KICK " + channelName + " " + targetNick + " : " + reason + "\r\n";
        const std::vector<Client*>& users = channel->getUsers();
        for (size_t i = 0; i < users.size(); ++i)
        {
            _server->sendToClient(users[i]->getFd(), kickMessage);
        }

        channel->removeUser(target);

        //채널 소멸 검사
        if (channel->getUsers().empty())
        {
            _server->removeChannel(channelName);
        }
}
