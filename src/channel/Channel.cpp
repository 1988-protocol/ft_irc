#include "channel/Channel.hpp"
#include "client/Client.hpp"

// 생성자: 모든 멤버 변수를 안전하게 초기화
Channel::Channel(std::string name)
    : m_name(name), m_topic(""), m_key(""), m_isInvite(false), m_isTopicOp(false), m_userLimit(0) {}

Channel::~Channel() {}

// --- Basic Getters & Setters ---

std::string Channel::getName() const
{
    return m_name;
}

std::string Channel::getTopic() const
{
    return m_topic;
}

std::string Channel::getKey() const
{
    return m_key;
}

// 현재 설정된 모드 상태를 문자열로 반환 (예: "+itk")
std::string Channel::getModeString() const
{
    std::string modes = "+";
    if (m_isInvite) modes += "i";
    if (m_isTopicOp) modes += "t";
    if (!m_key.empty()) modes += "k";
    if (m_userLimit > 0) modes += "l";
    
    return (modes == "+") ? "" : modes;
}

const std::map<Client*, bool>& Channel::getMembers() const
{
    return m_members;
}

void Channel::setTopic(std::string topic)
{
    m_topic = topic;
}

// --- User Management ---

void Channel::addUser(Client* client)
{
    if (!client)
        return;
    if (m_members.find(client) == m_members.end())
        m_members[client] = false;
}

void Channel::removeUser(Client* client)
{
    if (!client)
        return;
    m_members.erase(client); // 유저와 방장 권한이 동시에 삭제됨
    removeInvite(client); // 초대 목록에서도 삭제
}

bool Channel::isUserInChannel(Client* client) const
{
    return m_members.count(client) > 0;
}

// --- Key (+k) ---

bool Channel::isKeyModeActive() const
{
    return !m_key.empty();
}

bool Channel::checkKey(const std::string& key) const
{
    return m_key == key;
}

void Channel::setKey(const std::string& key)
{
    m_key = key;
}

void Channel::removeKey()
{
    m_key = "";
}

// --- Invite Only (+i) ---

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

// --- Topic Restriction (+t) ---

bool Channel::isTopicOpOnly() const
{
    return m_isTopicOp;
}

void Channel::setTopicOpOnly(bool flag)
{
    m_isTopicOp = flag;
}

// --- User Limit (+l) ---

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
    if (!hasUserLimit()) // 인원 제한 모드가 꺼져있으면 정원 초과일 리 없음
        return false;
        
    return m_members.size() >= static_cast<size_t>(m_userLimit);
}

// --- Operator (+o) ---

bool Channel::isOperator(Client* client) const
{
    std::map<Client*, bool>::const_iterator it = m_members.find(client);
    if (it != m_members.end())
        return it->second; // true/false 반환
    return false;
}

void Channel::addOperator(Client* client)
{
    if (isUserInChannel(client))
        m_members[client] = true;
}

void Channel::removeOperator(Client* client)
{
    if (isUserInChannel(client))
        m_members[client] = false;
}