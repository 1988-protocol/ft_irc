#include "Part.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"

Part::Part(Server* server) : ICommand(server) {}

Part::Part(const Part& other) : ICommand(other) {}

Part& Part::operator=(const Part& other)
{
    if (this != &other)
    {
        ICommand::operator=(other);
    }
    return *this;
}

Part::~Part() {}

void Part::execute(Client* sender, const std::vector<std::string>& params)
{
    if (!sender)
        return;

    // 1. 인자 개수 검사 (461 ERRm_NEEDMOREPARAMS)
    if (params.empty())
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 461 " + sender->getNickname() + " PART :Not enough parameters\r\n");
        return;
    }

    std::string channelName = params[0];

    // 2. 채널 존재 여부 확인 (403 ERRm_NOSUCHCHANNEL)
    Channel* channel = m_server->getChannel(channelName);
    if (!channel)
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 403 " + sender->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    // 3. 유저가 채널 멤버인지 확인 (442 ERRm_NOTONCHANNEL)
    if (!channel->isUserInChannel(sender))
    {
        m_server->sendToClient(sender->getFd(), ":ftm_irc 442 " + sender->getNickname() + " " + channelName + " :You're not on that channel\r\n");
        return;
    }

    // 4. 퇴장 메시지(Reason) 조립
    std::string partReason = "";
    if (params.size() >= 2)
    {
        partReason = " :" + params[1];
    }

    // 5. PART 메시지 브로드캐스트 (나가는 유저 포함 전원에게 전송)
    std::string partMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname()
                            + " PART " + channelName + partReason + "\r\n";

    const std::vector<Client*>& users = channel->getUsers();
    for (sizem_t i = 0; i < users.size(); ++i)
    {
        m_server->sendToClient(users[i]->getFd(), partMessage);
    }

    // 6. 채널 유저 및 방장/초대 목록 연쇄 제거 (removeUser 내부에서 연쇄 처리됨)
    channel->removeUser(sender);

    // 7. 빈 방 삭제 처리
    if (channel->getUsers().empty())
    {
        m_server->removeChannel(channelName);
    }
}