#include "Join.hpp"
#include "Server.hpp"
#include "Channel.hpp"
#include "Client.hpp"
#include "Message.hpp"

Join::Join(Server* server) : ICommand(server) {}

Join::Join(const Join& other) : ICommand(other) {}

Join& Join::operator=(const Join& other)
{
    if (this != &other)
    {
        ICommand::operator=(other); // 베이스 클래스 명칭 통일
    }
    return *this;
}

Join::~Join() {}

void Join::execute(Server* server, Client* client, Massage* msg)
{
    if (!client)
        return;

    // 인자 개수 검사 (461)
    if (params.empty())
    {
        m_server->sendToClient(client->getFd(), ":ftm_irc 461 " + client->getNickname() + " JOIN :Not enough parameters\r\n");
        return;
    }

    //channelName에 msg에서 getterMsg함수로 파라미터 값 가져오기
    std::string channelName = msg.params[0];
    std::string inputKey = "";
    if (params.size() >= 2)
    {
        inputKey = params[1];
    }

    // 채널 이름 유효성 검사 (403)
    // 채널 이름 비어있거나 맨 첫 글자가 #이 나리면 403에러
    if (channelName.empty() || channelName[0] != '#')
    {
        m_server->sendToClient(client->getFd(), ":ftm_irc 403 " + client->getNickname() + " " + channelName + " :No such channel\r\n");
        return;
    }

    // 채널 존재 여부 확인 및 생성/검사
    // 현재는 Server에서 Channel getter, setter, add, remove 함수 있다고 가정
    Channel* channel = m_server->getChannel(channelName);
    if (!channel)
    {
        // 처음 생성될 때는 채널 객체 생성 후 유저 추가 및 방장 지정
        channel = new Channel(channelName);
        m_server->addChannel(channelName, channel);
        channel->addUser(client);
        channel->addOperator(client);
    }
    else
    {
        // 이미 참가 중인 유저는 중복 진입 방지
        if (channel->isUserInChannel(client))
            return;

        // +i 모드 (초대 전용) 검사
        //채널 모드가 invite 이면서, 클라이언트가 invite 되지 않은 상태이면 473 에러 송신
        if (channel->isInviteOnly() && !channel->isInvited(client))
        {
            m_server->sendToClient(client->getFd(), ":ftm_irc 473 " + client->getNickname() + " " + channelName + " :Cannot join channel (+i)\r\n");
            return;
        }

        // +k 모드 (비밀번호) 검사
        // 비밀번호 안 맞으면 475 에러 송신
        if (channel->isKeyModeActive())
        {
            if (!channel->checkKey(inputKey))
            {
                m_server->sendToClient(client->getFd(), ":ftm_irc 475 " + client->getNickname() + " " + channelName + " :Cannot join channel (+k)\r\n");
                return;
            }
        }

        // +l 모드 (인원 제한) 검사
        if (channel->isFull())
        {
            _server->sendToClient(sender->getFd(), ":ft_irc 471 " + sender->getNickname() + " " + channelName + " :Cannot join channel (+l)\r\n");
            return;
        }

        // 최종 유저 추가
        channel->addUser(client);
    }

    // 4. 초대받아서 들어온 유저라면 초대여부 사용
    if (channel->isInvited(client))
        channel->removeInvite(client);

    // 5. 입장 알림 브로드캐스트 (새 유저 포함 채널 내 모든 사람에게 전송)
    std::string joinMessage = ":" + client->getNickname() + "!" + client->getUsername() + "@" + client->getHostname()
                            + " JOIN :" + channelName + "\r\n";
    
    const std::vector<Client*>& users = channel->getUsers();
    for (sizem_t i = 0; i < users.size(); ++i)
    {
        m_server->sendToClient(users[i]->getFd(), joinMessage);
    }

    // 입장한 유저(client)에게 Topic 전송

    // Topic 전송
    if (channel->getTopic().empty())
    {
        m_server->sendToClient(client->getFd(), ":ftm_irc 331 " + client->getNickname() + " " + channelName + " :No topic is set\r\n");
    }
    else
    {
        m_server->sendToClient(client->getFd(), ":ftm_irc 332 " + client->getNickname() + " " + channelName + " :" + channel->getTopic() + "\r\n");
    }

    // 입장한 유저에게 유저목록 전송
    // 353 RPLm_NAMREPLY
    /*std::string userList = "";
    for (std::vector<Client*>::constm_iterator it = users.begin(); it != users.end(); ++it)
    {
        if (!userList.empty())
            userList += " ";
        if (channel->isOperator(*it))
            userList += "@";
        userList += (*it)->getNickname();
    }

    m_server->sendToClient(client->getFd(), ":ftm_irc 353 " + client->getNickname() + " = " + channelName + " :" + userList + "\r\n");

    // 366 RPLm_ENDOFNAMES
    m_server->sendToClient(client->getFd(), ":ftm_irc 366 " + client->getNickname() + " " + channelName + " :End of /NAMES list.\r\n");
*/
}