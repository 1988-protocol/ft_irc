#include "server/Server.hpp"

#include <iostream>
#include <cstddef>		// std::size_t
#include <cerrno>		// errno, EINTR, EAGAIN
#include <cstring>		// memset
#include <csignal>		// sigaction, signal
#include <unistd.h>		// close, recv, send
#include <sys/socket.h>	// accept, recv, send
#include <netinet/in.h>	// sockaddr_in
#include <arpa/inet.h>	// inet_ntoa

// 시그널 핸들러가 건드릴 전역 플래그(클래스 static). 처음엔 실행 중.
bool	Server::_running = true;

// ===========================================================================
// 생성자 / 소멸자
// ===========================================================================
Server::Server(int port, const std::string &password)
	: _port(port), _password(password)
{
	// 1) 리스닝 창구를 연다(실패하면 여기서 예외 -> main이 잡음).
	int	listenFd = _listener.createListener(_port);

	// 2) 그 창구를 '단일 poll 목록'의 첫 항목으로 등록한다.
	_poll.add(listenFd);

	// 3) 시그널 설치: Ctrl-C(SIGINT)/종료(SIGTERM)에서 곱게 멈추기.
	//    SIGPIPE는 무시 -> 끊긴 상대에게 send해도 프로세스가 안 죽는다.
	struct sigaction	sa;
	std::memset(&sa, 0, sizeof(sa));
	sa.sa_handler = &Server::signalHandler;
	sigemptyset(&sa.sa_mask);
	sa.sa_flags = 0;
	sigaction(SIGINT, &sa, 0);
	sigaction(SIGTERM, &sa, 0);
	signal(SIGPIPE, SIG_IGN);

	std::cout << "[server] 포트 " << _port << " 에서 대기 시작" << std::endl;
}

Server::~Server()
{
	// 남아있는 모든 클라이언트를 닫고 메모리 해제.
	for (std::map<int, Client*>::iterator it = _clients.begin();
		 it != _clients.end(); ++it)
	{
		close(it->first);
		delete it->second;
	}
	_clients.clear();
	// 리스닝 소켓은 Socket 소멸자가 알아서 close 한다.
}

// ===========================================================================
// 메인 루프
// ===========================================================================
void	Server::run()
{
	while (_running)
	{
		// (1) 단 하나의 poll: 이벤트가 생길 때까지 잠든다.
		if (_poll.wait() < 0)
		{
			// 시그널 때문에 깬 것(EINTR)은 정상 -> 다시 루프.
			if (errno == EINTR)
				continue;
			break;	// 그 외 진짜 에러면 루프 종료.
		}

		// (2) 목록을 처음부터 끝까지 훑으며 "무슨 일이 났나" 확인.
		for (std::size_t i = 0; i < _poll.size(); ++i)
		{
			int		fd = _poll.fdAt(i);
			short	re = _poll.reventsAt(i);	// 실제로 일어난 이벤트

			if (re == 0)
				continue;	// 이 fd는 아무 일 없음.

			int	listenFd = _listener.getFd();

			// (2-a) 에러/끊김: 클라이언트면 정리.
			if ((re & (POLLERR | POLLHUP | POLLNVAL)) && fd != listenFd)
			{
				disconnectClient(fd);
				--i;	// 목록이 한 칸 줄었으니 인덱스 보정.
				continue;
			}

			// (2-b) 읽을 게 있다(POLLIN).
			if (re & POLLIN)
			{
				if (fd == listenFd)
					acceptNewClient();		// 창구면 -> 새 손님 받기
				else
				{
					receiveFromClient(fd);	// 클라면 -> 데이터 읽기
					// 읽는 중 끊겨서 사라졌으면 인덱스 보정 후 넘어감.
					if (_clients.find(fd) == _clients.end())
					{ --i; continue; }
				}
			}

			// (2-c) 보낼 수 있다(POLLOUT) -> 출력 버퍼 비우기.
			if (re & POLLOUT)
			{
				if (_clients.find(fd) != _clients.end())
					sendToClient(fd);
			}
		}
	}
	std::cout << "\n[server] 종료합니다." << std::endl;
}

// ===========================================================================
// 새 접속 받기
// ===========================================================================
void	Server::acceptNewClient()
{
	// 한 번의 POLLIN에 여러 접속이 몰릴 수 있으니 더 없을 때까지 반복 accept.
	while (true)
	{
		struct sockaddr_in	addr;
		socklen_t			len = sizeof(addr);
		int	clientFd = accept(_listener.getFd(),
							   reinterpret_cast<struct sockaddr*>(&addr), &len);
		if (clientFd < 0)
		{
			// 더 이상 대기 중인 접속이 없음 -> 정상 종료.
			if (errno == EAGAIN || errno == EWOULDBLOCK)
				break;
			break;	// 그 외 에러도 이번 라운드는 중단.
		}

		// 새 클라 소켓도 반드시 논블로킹으로.
		Socket::setNonBlocking(clientFd);

		std::string	ip = inet_ntoa(addr.sin_addr);
		Client	*client = new Client(clientFd, ip);
		_clients[clientFd] = client;	// fd -> Client 등록
		_poll.add(clientFd);			// 감시 목록에 추가

		std::cout << "[server] 새 접속: " << ip
				  << " (fd " << clientFd << ")" << std::endl;
	}
}

// ===========================================================================
// 클라이언트에서 읽기
// ===========================================================================
void	Server::receiveFromClient(int fd)
{
	char	buf[4096];
	ssize_t	n = recv(fd, buf, sizeof(buf), 0);

	if (n == 0)
	{
		// 상대가 정상적으로 연결을 닫음.
		disconnectClient(fd);
		return;
	}
	if (n < 0)
	{
		// 논블로킹에서 EAGAIN은 "지금은 읽을 것 없음" -> 그냥 넘어감.
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		disconnectClient(fd);	// 진짜 에러면 정리.
		return;
	}

	Client	*client = _clients[fd];
	// 받은 바이트를 입력 버퍼에 쌓는다(아직 부분 패킷일 수 있음).
	client->appendToInBuffer(std::string(buf, n));

	// 완성된 줄이 있으면 있는 대로 모두 꺼내 처리.
	std::string	line;
	while (client->extractLine(line))
		handleLine(client, line);

	// 처리 결과 "끊어야 함"이고 보낼 것도 다 비었으면 즉시 종료.
	if (client->needsDisconnect() && !client->hasPendingOutput())
		disconnectClient(fd);
}

// ===========================================================================
// ★ 한 줄(명령) 처리 — Week 1은 '에코'
// ===========================================================================
void	Server::handleLine(Client *client, const std::string &line)
{
	// ─────────────────────────────────────────────────────────────
	// ★★★ Week 2 교체 지점 ★★★
	// 지금은 받은 줄을 그대로 되돌려 보낸다(에코).
	// 나중엔 이 한 줄이  ->  _parser.parse(client, line);  로 바뀐다.
	// 즉 '에코' 자리에 '파서+명령 실행'이 꽂히면 IRC 서버가 된다.
	// ─────────────────────────────────────────────────────────────
	std::cout << "[recv fd " << client->getFd() << "] " << line << std::endl;

	std::string	reply = "echo: " + line + "\r\n";

	// 직접 send 하지 않는다! 출력 버퍼에 쌓고, 이 fd의 POLLOUT을 켠다.
	// -> 다음 poll이 "보낼 수 있다"고 알리면 그때 sendToClient가 보낸다.
	client->appendToOutBuffer(reply);
	_poll.setWritable(client->getFd(), true);
}

// ===========================================================================
// 출력 버퍼 비우기(flush)
// ===========================================================================
void	Server::sendToClient(int fd)
{
	Client		*client = _clients[fd];
	std::string	&out = client->getOutBuffer();

	if (out.empty())
	{
		// 보낼 게 없으면 POLLOUT을 꺼서 poll이 헛되이 안 깨게 한다.
		_poll.setWritable(fd, false);
		return;
	}

	ssize_t	n = send(fd, out.c_str(), out.size(), 0);
	if (n < 0)
	{
		// 지금은 못 보냄 -> 다음 POLLOUT에서 다시 시도.
		if (errno == EAGAIN || errno == EWOULDBLOCK)
			return;
		disconnectClient(fd);	// 진짜 에러면 정리.
		return;
	}

	// 보낸 만큼만 앞에서 지운다(부분 전송 대비). 나머지는 다음에.
	out.erase(0, n);

	if (out.empty())
	{
		_poll.setWritable(fd, false);	// 다 보냈으니 쓰기 감시 끔
		// "끊어야 함" 표시였다면 이제 다 보냈으니 종료.
		if (client->needsDisconnect())
			disconnectClient(fd);
	}
}

// ===========================================================================
// 연결 끊고 정리
// ===========================================================================
void	Server::disconnectClient(int fd)
{
	std::map<int, Client*>::iterator	it = _clients.find(fd);
	if (it == _clients.end())
		return;

	std::cout << "[server] 연결 종료 (fd " << fd << ")" << std::endl;

	_poll.remove(fd);		// 감시 목록에서 빼고
	close(fd);				// 소켓 닫고
	delete it->second;		// Client 메모리 해제
	_clients.erase(it);		// 맵에서 제거
}

// ===========================================================================
// 시그널 핸들러
// ===========================================================================
void	Server::signalHandler(int sig)
{
	(void)sig;			// 어떤 시그널인지는 안 따짐
	_running = false;	// 루프가 이걸 보고 스스로 빠져나온다.
}
