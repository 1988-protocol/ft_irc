#ifndef CHANNELm_HPP
# define CHANNELm_HPP

#include <string>
#include <vector>

class Client;

class Channel
{
private:
    std::string m_name;       // 채널 이름
    std::string m_topic;      // 채널 주제
    std::string m_key;        // 채널 비밀번호 (+k)
    bool        m_isInvite;   // 초대 전용 여부 (+i)
    bool        m_isTopicOp;  // 방장만 토픽 변경 가능 여부 (+t)
    int         m_userLimit;  // 인원 제한 (+l, 0이면 제한 없음)

    std::vector<Client *> m_users;        // 참여자 목록
    std::vector<Client *> m_operators;    // 운영자 목록 (+o)
    std::vector<Client *> m_invitedUsers; // 초대받은 유저 목록 (+i)

public:
    Channel(std::string name);
    ~Channel();

    std::string getName() const;
    std::string getTopic() const;
    std::string getKey() const;
    std::string getModeString() const; // 현재 모드 상태 문자열 반환 (예: "+itk")
    const std::vector<Client *> getUsers() const;
    const std::vector<Client *> getOperators() const;

    void setTopic(std::string topic);
    void addUser(Client *client);
    void removeUser(Client *client);

    // Key (+k) 관련
    bool isKeyModeActive() const;
    bool checkKey(const std::string& key) const;
    void setKey(const std::string& key);
    void removeKey();

    // Invite Only (+i) 관련
    bool isInviteOnly() const;
    void setInviteOnly(bool flag);
    bool isInvited(Client *client) const;
    void addInvite(Client *client);
    void removeInvite(Client *client);

    // Topic Restriction (+t) 관련
    bool isTopicOpOnly() const;
    void setTopicOpOnly(bool flag);

    // User Limit (+l) 관련
    bool hasUserLimit() const;
    int  getUserLimit() const;
    void setUserLimit(int limit);
    void removeUserLimit();
    bool isFull() const;

    // Operator (+o) 관련
    bool isOperator(Client* client) const;
    void addOperator(Client* client);
    void removeOperator(Client* client);
};

#endif