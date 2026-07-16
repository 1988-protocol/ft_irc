#include "Channel.hpp"

Channel::Channel(std::string name) : _name(name), _topic("") {}

Channel::~Channel() {}

std::string Channel::getName() const
{
	return _name;
}

std::string Channel::getTopic() const
{
	return _topic;
}

std::vector<Client *> Channel::getUsers() const
{
	return _users;
}

std::vector<Client *> Channel::getOperators() const
{
	return _operators;
}

void Channel::setTopic(std::string topic)
{
	_topic = topic;
}

void Channel::addUser(Client *client)
{
	if (!client)
		return;

	for (size_t i = 0; i < _users.size(); ++i)
	{
		if (_users[i] == client)
			return; // 이미 참여한 User인 경우 추가하지 않음
	}
	_users.push_back(client);
}

void Channel::removeUser(Client *client)
{
	if (!client)
		return;

	for (size_t i = 0; i < _users.size(); ++i)
	{
		if (_users[i] == client)
		{
			_users.erase(_users.begin() + i);
			break;
		}
	}
	removeOperator(client);
}

void Channel::addOperator(Client *client)
{
	if (!client)
		return;

	for (size_t i = 0; i < _operators.size(); ++i)
	{
		if (_operators[i] == client)
			return; // 이미 운영자인 경우 추가하지 않음
	}
	_operators.push_back(client);
}

void Channel::removeOperator(Client *client)
{
	if (!client)
		return;

	for (size_t i = 0; i < _operators.size(); ++i)
	{
		if (_operators[i] == client)
		{
			_operators.erase(_operators.begin() + i);
			break;
		}
	}
}

bool Channel::isOperator(Client *client) const
{
	if (!client)
		return false;

	for (size_t i = 0; i < _operators.size(); ++i)
	{
		if (_operators[i] == client)
			return true; // 해당 User가 운영자인 경우 true 반환
	}
	return false; // 해당 User가 운영자가 아닌 경우 false 반환
}