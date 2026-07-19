#include "Server.hpp"
#include <iostream>
#include <sstream>

void Server::executeJoin(Client* sender, const std::string& channelName)
{
	if (!sender)
		return;

	if (channelName.empty() || channelName[0] != '#')
	{
		// ERR_NOSUCHCHANNEL (403)
		return ;
	}
	//채널이 존재하지 않으면, 새로 생성, 새로 생성했으니 생성된 채널의 클라이언트(sender)를 관리자로 추가
	if (_channels.find(channelName) == _channels.end())
	{
		Channel* newChannel = new Channel(channelName);
		_channels[channelName] = newChannel;

		newChannel->addOperator(sender);
	}
	//채널이 존재하면, 채널에 있는 유저 명단을 가져와서, sender가 이미 참여 중인지 확인
	const std::vector<Client*>& users = channel->getUsers();
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] == sender)
			return ; // 이미 참여 중인 경우, 아무 작업도 수행하지 x
	}

	channel->addUser(sender);

	//JOIN 했다!고 채널 내 모든 User에게 알림 보냄(본인 포함)
	//형식: :<sender_nick>!<sender_user>@<sender_host> JOIN <channel_name>
	std::string joinMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname() + " JOIN " + channelName;
	for (size_t i = 0; i < users.size(); ++i)
	{
		//클라이언트에게 메시지 전송하는 함수 (서버에서 구현되어 있으면 쓰고, 아니면 따로 구현하기)

	}
}

void Server::executePart(Client* sender, const std::string& channelName)
{

	if (!sender)
		return;
	
	if (_channels.find(channelName) == _channels.end())
	{
		//ERR_NOSUCHCHANNEL (403)
		return ;
	}

	Channel* channel = _channels[channelName];

	const std::vector<Client*>& users = channel->getUsers();
	bool isUserInChannel = false; //채널에 있는지 아닌지 확인하기 위해 선언
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] == sender)
		{
			isUserInChannel = true;
			break;
		}
	}

	if (!isUserInChannel)
	{
		//ERR_NOTONCHANNEL (442)
		return ;
	}

	//PART 했다고 채널 내 모든 User에게 알림 보냄(본인 포함)
	//형식체크 다시
	std::string partMessage = ":" + sender->getNickname() + "!" + sender->getUsername() + "@" + sender->getHostname() + " PART " + channelName;
	for (size_t i = 0; i < users.size(); ++i)
	{
		//클라이언트에게 메시지 전송하는 함수 (서버에서 구현되어 있으면 쓰고, 아니면 따로 구현하기)
	}

	channel->removeUser(sender);

	//유저가 사라지면서 채널까지 사라지게 되면 채널 삭제하고 메모리 해제.
	if (channel->getUsers().empty())
	{
		_channels.erase(channelName);
		delete channel;
	}
}


void Server::executeKick(Client* sender, Channel* channel, Client* target, const std::string& reason)
{
	if (!sender || !channel || !target)
		return;
	
	std::string channelName = channel->getName();

	//KICK 명령어를 실행하는 sender가 채널에 참여하고 있는지 확인
	const std::vector<Client*>& users = channel->getUsers();
	bool isSenderMember = false;
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] == sender)
		{
			isSenderMember = true;
			break;
		}
	}

	//만약 채널 멤버가 아니면 442에러코드 
	if (!isSenderMember)
	{
		//ERR_NOTONCHANNEL (442)
		return ;
	}

	//채널 멤버라면, 그 채널의 operator인지 확인
	//operator가 아니라면 482 에러코드
	if (!channel->isOperator(sender))
	{
		//ERR_CHANOPRIVSNEEDED (482)
		return ;
	}
	//operator라면, 강퇴할 대상이 채널에 참여하고 있는지 확인
	bool isTargetMember = false;
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] == target)
		{
			isTargetMember = true;
			break;
		}
	}

	//강퇴할 대상이 채널에 없으면, 441 에러코드
	if (!isTargetMember)
	{
		//ERR_USERNOTINCHANNEL (441)
		return ;
	}
	//강퇴할 대상이 채널에 있으면, 강퇴시킨 후 채널에 있는 모든 유저에게 강퇴사항 알림메시지 전송
	std::string kickMessage = ":" + sender->getNickname() + "!" + "KICK" + channelName + " " + target->getNickname() + " :" + "\r\n";
	for (size_t i = 0; i < users.size(); ++i)
	{
		this->sendToClient(users[i]->getFd(), kickMessage);
	}
	channel->removeUser(target);
	//강퇴 후에 채널에 남은 유저 없으면, 채널 삭제 후 메모리 해제
	if (channel->getUsers().empty())
	{
		_channels.erase(channelName);
		delete channel;
		//출력 메시지는 다른 파트와 통일성 있게 수정하기
		std::cout << "Channel " << channelName << " has been deleted due to no remaining users." << std::endl;
	}
}

void Server::executeChannelPrivmsg(Client* sender, const std::string& channelName, const std::string& message)
{
	if (!sender)
		return;
	//채널 존재하는지 체크 후 없으면 403 에러코드
	if (_channels.find(channelName) == _channel.end())
	{
		//ERR_NOSUCHCHANNEL (403)
		return ;
	}

	//채널 존재하면, 메시지를 보내는 유저가 채널에 참여하고 있는지 확인 (외부인 차단)
	const std::vector<Client*>& users = channel->getUsers();
	bool isMember = false;
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] == sender)
		{
			isMember = true;
			break;
		}
	}

	//채널 참여자 아니면 404 에러코드
	if (!isMember)
	{
		//ERR_CANNOTSENDTOCHAN (404)
		return ;
	}

	//채널 참여자라면 본인 제외 채널 유저들에게 메시지 전송
	std::string packet = ":" + sender->getNickname() + " PRIVMSG " + channelName + " :" + message + "\r\n";
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] != sender)
		{
			this->sendToClient(users[i]->getFd(), packet);
		}
	}
}

void Server::executeInvite(Client* sender, Channel* channel, Client* target)
{
	if (!sender || !channel || !target)
		return;
	std::string channelName = channel->getName();
	
	//초대 보내는 유저가 채널에 참여하고 있는지 확인
	const std::vector<Client*>& users = channel->getUsers();
	bool isSenderMember = false;
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] == sender)
		{
			isSenderMember = true;
			break;
		}
	}

	//채널 참여자 아니면 442 에러코드
	if (!isSenderMember)
	{
		//ERR_NOTONCHANNEL (442)
		return ;
	}

	//초대 전용 모드(+i) 확인하고 방장 권한 있는지 확인
	if (channel->isInviteOnly() && !channel->isOperator(sender))
	{
		//ERR_CHANOPRIVSNEEDED (482)
		return ;
	}
	//초대받는 유저가 채널에 참여하고 있는지 확인
	//초대하려는 유저가 이미 채널에 속한 유저이면 443 에러코드
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] == target)
		{
			//ERR_USERONCHANNEL (443)
			return ;
		}
	}
	
	//초대 list에 추가
	channel->addInvite(target->getNickname());

	//초대 보낸 client한테 성공응답코드 보내기
	//초대 받은 client에게 "sender가 너를 초대했다"고 알림 보내기
	
}

void Server::executeTopic(Client* sender, Channel* channel, const std::string& newTopic)
{
	if (!sender || !channel)
		return;

	std::string channelName = channel->getName();

	//발신자가 이 채널의 멤버인지 확인
	const std::vector<Client*>& users = channel->getUsers();
	bool isMember = false;
	for (size_t i = 0; i < users.size(); ++i)
	{
		if (users[i] == sender)
		{
			isMember = true;
			break;
		}
	}

	//아니라면 442 에러코드
	if (!isMember)
	{
		//ERR_NOTONCHANNEL (442)
		return ;
	}
	
	//새로운 토픽이 없으면, 현재 토픽을 확인
	if (!hasNewTopic)
	{
		//현재 토픽이 비어있으면, 331 에러코드
		if (channel->getTopic().empty())
		{
			//ERR_NOTOPIC (331)
			return ;
		}

		//현재 토픽이 있으면, 332 에러코드
		else
		{
			//RPL_TOPIC (332)
			return ;
		}
	}

	//토픽 제한 모드(+t)인지 확인 후, t이면 방장인지 확인
	//방장이 아니면 482 에러코드
	if (channel->isTopicRestricted() && !channel->isOperator(sender))
	{
		//ERR_CHANOPRIVSNEEDED (482)
		return ;
	}

	//방장이 맞다면, 토픽 변경 후, 채널에 있는 유저에게 토픽 변경 알림
	channel->setTopic(newTopic);

	std::string topicMessage = ":" + sender->getNickname() + " TOPIC " + channelName + " :" + "\r\n";
	for (size_t i = 0; i < users.size(); ++i)
	{
		this->sendToClient(users[i]->getFd(), topicMessage);
	}
}

