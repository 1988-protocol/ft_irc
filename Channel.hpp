#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <string>
#include <vector>

class Client;

class Channel
{
private:
	std::string _name; // 채널 이름
	std::string _topic; // 채널 주제 topic 명령어로 지정
	std::vector<Client *> _users; // 채널에 참여한 User 목록
	std::vector<Client *> _operators; // 채널 운영자 목록 (서버가 하나 있다는 말이 채널이 꼭 하나여야 한다는 말이 아님)

public:
	Channel(std::string name);
	~Channel();

	std::string getName() const; // 채널 이름 반환
	std::string getTopic() const; // 채널 topic 반환
	std::vector<Client *> getUsers() const; // 채널에 참여한 User 목록 반환
	std::vector<Client *> getOperators() const; // 채널 operator 목록 반환

	void setTopic(std::string topic); // 채널 topic 설정
	void addUser(Client *client); // 채널에 User 추가
	void removeUser(Client *client); // 채널에서 User 제거

	void addOperator(Client *client); // 채널 operator 추가
	void removeOperator(Client *client); // 채널 operator 제거

	bool isOperator(Client *client) const; // 해당 User가 채널 operator인지 확인
};

#endif