#include "channel/Channel.hpp"
#include "client/Client.hpp"

Channel::Channel(std::string name)
    : m_name(name), m_topic(""), m_key(""), m_isInvite(false), m_isTopicOp(false), m_userLimit(0) {}

Channel::~Channel() {}

std::string Channel::getName() const
{
    return m_name;
}

const std::map<Client*, bool>& Channel::getMembers() const
{
    return m_members;
}

// 현재 설정된 모드 상태를 문자열로 반환
std::string Channel::getModeString() const
{
    std::string modes = "+";
    if (m_isInvite) modes += "i";
    if (m_isTopicOp) modes += "t";
    if (!m_key.empty()) modes += "k";
    if (m_userLimit > 0) modes += "l";
    if (modes == "+")
        return "";
    return modes;
}

// 채널 Member 관리

void Channel::addMember(Client* client)
{
    if (!client)
        return;
    if (m_members.find(client) == m_members.end())
        m_members[client] = false;
}

void Channel::removeMember(Client* client)
{
    if (!client)
        return;
    m_members.erase(client); // 유저와 방장 권한이 동시에 삭제됨
    removeInvite(client); // 초대 목록에서도 삭제
}

bool Channel::isMember(Client* client) const
{
    return m_members.count(client) > 0;
}

// 채널 Key 관리

std::string Channel::getKey() const
{
    return m_key;
}

void Channel::setKey(const std::string& key)
{
    m_key = key;
}

bool Channel::isKeyModeActive() const
{
    return !m_key.empty();
}

bool Channel::checkKey(const std::string& key) const
{
    return m_key == key;
}

void Channel::removeKey()
{
    m_key = "";
}

// 채널 Invite 관리

bool Channel::isInviteOnly() const
{
    return m_isInvite;
}

void Channel::setInviteOnly(bool flag)
{
    m_isInvite = flag;
}

bool Channel::isInvited(Client* client) const
{
    if (!client)
        return false;
    return m_invitedUsers.count(client) > 0;
}


void Channel::addInvite(Client* client)
{
    if (!client)
        return;
    m_invitedUsers.insert(client);
}

void Channel::removeInvite(Client* client)
{
    if (!client)
        return;
    m_invitedUsers.erase(client);
}

// 채널 topic 관리

std::string Channel::getTopic() const
{
    return m_topic;
}

void Channel::setTopic(std::string topic)
{
    m_topic = topic;
}

bool Channel::isTopicOpOnly() const
{
    return m_isTopicOp;
}

void Channel::setTopicOpOnly(bool flag)
{
    m_isTopicOp = flag;
}

// 채널 유저수 관리 (+l)

bool Channel::hasUserLimit() const
{
    return m_userLimit > 0;
}

int Channel::getUserLimit() const
{
    return m_userLimit;
}

void Channel::setUserLimit(int limit)
{
    m_userLimit = limit;
}

void Channel::removeUserLimit()
{
    m_userLimit = 0;
}

bool Channel::isFull() const
{
    if (!hasUserLimit()) // 인원 제한 모드가 꺼져있으면 정원 초과 x
        return false;
        
    return m_members.size() >= static_cast<size_t>(m_userLimit);
}

// 채널 방장 관리

bool Channel::isOperator(Client* client) const
{
    std::map<Client*, bool>::const_iterator it = m_members.find(client);
    if (it != m_members.end())
        return it->second; // true/false 반환
    return false;
}

void Channel::addOperator(Client* client)
{
    if (isMember(client))
        m_members[client] = true;
}

void Channel::removeOperator(Client* client)
{
    if (isMember(client))
        m_members[client] = false;
}