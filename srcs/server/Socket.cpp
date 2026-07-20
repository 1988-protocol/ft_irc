#include "server/Socket.hpp"

#include <stdexcept>	// std::runtime_error
#include <cstring>		// memset
#include <unistd.h>		// close
#include <fcntl.h>		// fcntl, O_NONBLOCK
#include <sys/socket.h>	// socket, setsockopt, bind, listen
#include <netinet/in.h>	// sockaddr_in, htons, htonl

Socket::Socket() : _fd(-1)
{
}

Socket::~Socket()
{
	// 열려 있으면 닫는다(자원 누수 방지).
	if (_fd >= 0)
		close(_fd);
}

/*
 * createListenㅇㅇer
 * --------------
 * "창구를 여는" 5단계 절차. 하나라도 실패하면 예외를 던져서
 * main이 잡아 깔끔히 종료하게 한다(지침: 어떤 상황에도 크래시 금지).
 */
int	Socket::createListener(int port)
{
	// 1) 소켓 생성: IPv4(AF_INET) + TCP(SOCK_STREAM)
	_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (_fd < 0)
		throw std::runtime_error("socket() 실패");

	// 2) SO_REUSEADDR: 서버를 껐다 바로 켤 때 "주소가 이미 사용 중" 에러 방지
	int	opt = 1;
	if (setsockopt(_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
		throw std::runtime_error("setsockopt() 실패");

	// 3) 논블로킹으로 전환(지침: 모든 fd는 논블로킹)
	setNonBlocking(_fd);

	// 4) bind: "0.0.0.0:port"에 이 소켓을 묶는다(모든 인터페이스에서 수신)
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);			// 0.0.0.0
	addr.sin_port = htons(static_cast<uint16_t>(port));	// 호스트->네트워크 바이트순서
	if (bind(_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
		throw std::runtime_error("bind() 실패 (포트가 사용 중?)");

	// 5) listen: 이제부터 접속 요청을 받겠다고 선언
	//SOMAXCONN의 의미
	//SOMAXCONN은 Socket Maximum Connections의 약자로, 
	//<sys/socket.h> 헤더 파일에 정의되어 있는 매크로 상수입니다.
	// 이 값을 listen()의 두 번째 인자로 넘겨주면, 운영체제(OS)에게 
	//"이 시스템(운영체제)이 허용하는 가장 큰 사이즈로 대기열(Backlog Queue)을 만들어줘!"라고 요청하는 것입니다.
	if (listen(_fd, SOMAXCONN) < 0)
		throw std::runtime_error("listen() 실패");

	return _fd;
}

/*
 * setNonBlocking
 * --------------
 * 지침이 허용한 유일한 형태. 다른 플래그를 OR하지 않는다.
 * 논블로킹이면 recv/send가 "지금 할 게 없으면" 기다리지 않고 바로 반환한다.
 */
void	Socket::setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl(O_NONBLOCK) 실패");
}

int	Socket::getFd() const
{
	return _fd;
}
