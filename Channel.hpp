#ifndef CHANNEL_HPP
# define CHANNEL_HPP

#include <string>
#include <vector>

class Channel
{
private:
	std::string _name;
	std::string _topic;
	std::vector<Client *> _users;
	std::vector<Client *> _operators;
	std::vector<std::string> _bannedUsers;

public:
	Channel(std::string name);
	~Channel();

	std::string getName() const;
	std::string getTopic() const;
	std::vector<Client *> getUsers() const;
	std::vector<Client *> getOperators() const;
	std::vector<std::string> getBannedUsers() const;

	void setTopic(std::string topic);
	void addUser(Client *client);
	void removeUser(Client *client);
	void addOperator(Client *client);
	void removeOperator(Client *client);
	void banUser(std::string username);
	void unbanUser(std::string username);
	bool isUserInChannel(Client *client) const;
};

#endif