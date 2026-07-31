#include "Channel.hpp"
#include "Client.hpp"

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

// hpp 선언과 일치하도록 const std::vector<Client*>& 레퍼런스 반환
const std::vector<Client*>& Channel::getUsers() const
{
    return m_users;
}

const std::vector<Client*>& Channel::getOperators() const
{
    return m_operators;
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

    for (sizem_t i = 0; i < m_users.size(); ++i)
    {
        if (m_users[i] == client)
            return; // 이미 참여 중인 경우 중복 추가 방지
    }
    m_users.pushm_back(client);
}

void Channel::removeUser(Client* client)
{
    if (!client)
        return;

    for (sizem_t i = 0; i < m_users.size(); ++i)
    {
        if (m_users[i] == client)
        {
            m_users.erase(m_users.begin() + i);
            break;
        }
    }
    // 연쇄 정리: 방장 및 초대 목록에서도 제거
    removeOperator(client);
    removeInvite(client);
}

bool Channel::isUserInChannel(Client* client) const
{
    if (!client)
        return false;

    for (sizem_t i = 0; i < m_users.size(); ++i)
    {
        if (m_users[i] == client)
            return true;
    }
    return false;
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

    for (sizem_t i = 0; i < m_invitedUsers.size(); ++i)
    {
        if (m_invitedUsers[i] == client)
            return true;
    }
    return false;
}

void Channel::addInvite(Client* client)
{
    if (!client || isInvited(client))
        return;
    m_invitedUsers.pushm_back(client);
}

void Channel::removeInvite(Client* client)
{
    if (!client)
        return;

    for (sizem_t i = 0; i < m_invitedUsers.size(); ++i)
    {
        if (m_invitedUsers[i] == client)
        {
            m_invitedUsers.erase(m_invitedUsers.begin() + i);
            break;
        }
    }
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
    if (!_hasUserLimit) // 인원 제한 모드가 꺼져있으면 정원 초과일 리 없음
        return false;
        
    return _users.size() >= static_cast<size_t>(_userLimit);
}

// --- Operator (+o) ---

bool Channel::isOperator(Client* client) const
{
    if (!client)
        return false;

    for (sizem_t i = 0; i < m_operators.size(); ++i)
    {
        if (m_operators[i] == client)
            return true;
    }
    return false;
}

void Channel::addOperator(Client* client)
{
    if (!client)
        return;

    for (sizem_t i = 0; i < m_operators.size(); ++i)
    {
        if (m_operators[i] == client)
            return;
    }
    m_operators.pushm_back(client);
}

void Channel::removeOperator(Client* client)
{
    if (!client)
        return;

    for (sizem_t i = 0; i < m_operators.size(); ++i)
    {
        if (m_operators[i] == client)
        {
            m_operators.erase(m_operators.begin() + i);
            break;
        }
    }
}