#include "Kick.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Kick::Kick(Server* server) : ICommand(server) {}

Kick::Kick(const Kick& other) : ICommand(other) {}

Kick& Kick::operator=(const Kick& other)
{
    if (this != &other)
    {
        ICommand::operator=(other);
    }
    return *this;
}

Kick::~Kick() {}

void Kick::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    // 1. 인자 개수 검사 (461 ERRm_NEEDMOREPARAMS)
    if (params.size() < 2)
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 461 " + sender->getNickname() + " KICK :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];
    std::string targetNick = params[1];
    std::string reason = (params.size() >= 3) ? params[2] : "No reason specified";

    // 2. 채널 존재 여부 확인 (403 ERRm_NOSUCHCHANNEL)
    Channel* channel = m_server->getChannel(channelName);
    if (!channel)
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 403 " + sender->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    // 3. 명령 내린 sender가 채널 멤버인지 확인 (442 ERRm_NOTONCHANNEL)
    if (!channel->isUserInChannel(sender))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 442 " + sender->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // 4. sender가 operator(방장)인지 확인 (482 ERRm_CHANOPRIVSNEEDED)
    if (!channel->isOperator(sender))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 482 " + sender->getNickname() + " " + channelName + " :You're not channel operator\r\n");
        return;
    }

    // 5. 강퇴 대상(target) 존재 및 채널 참가 여부 확인 (441 ERRm_USERNOTINCHANNEL)
    Client* target = m_server->getClientByNick(targetNick);
    if (!target || !channel->isUserInChannel(target))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 441 " + sender->getNickname() + " " + targetNick + " " + channelName + " :They aren't on that channel\r\n");
        return;
    }

    // 6. 강퇴 메시지 전송 (나가는 target 포함 전체 브로드캐스트)
    std::string kickMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                            + " KICK " + channelName + " " + targetNick + " :" + reason + "\r\n"; // 🌟 " :" 공백 수정
    
    const std::vector<Client*>& users = channel->getUsers();
    for (sizem_t i = 0; i < users.size(); ++i)
    {
        m_server->sendToClient(users[i]->getFd(), kickMessage);
    }

    // 7. 채널에서 target 제거 (Channel.cpp 내부에서 방장/초대 목록 연쇄 정리)
    channel->removeUser(target);

    // 8. 채널 소멸 검사 (빈 방 삭제)
    if (channel->getUsers().empty())
    {
        m_server->removeChannel(channelName);
    }
}
