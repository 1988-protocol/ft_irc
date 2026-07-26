#include "../include/server/Socket.hpp"

#include <unistd.h>     // close;
#include <cstring>		// memset
#include <sys/socket.h> // socket setsockopt
#include <stdexcept>	// std::runtime_error
#include <fcntl.h>      // fcntl
#include <netinet/in.h> // sockaddr_in hton htol

// ────────────────────────────────────────────────────────
// ocf
// ────────────────────────────────────────────────────────

Socket::Socket() : m_fd(-1) {}

Socket::~Socket()
{
    // fd 닫아서 누수 방지
    if (m_fd >= 0)
        close(m_fd);
}

Socket::Socket(Socket const &other) 
{
	*this = other;
}

Socket &Socket::operator=(Socket const &other)
{
	if (this != &other)
	{
		this->m_fd = other.m_fd;
	}
	return *this;
}

// ────────────────────────────────────────────────────────
// 내부 함수
// ────────────────────────────────────────────────────────

int Socket::createListener(int port)
{
    // 1) 소켓 생성: IPv4(AF_INET) + TCP(SOCK_STREAM)
	m_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (m_fd < 0)
		throw std::runtime_error("socket() 실패");

    // 2) setsockopt로 소켓 옵션 변경 : 
    // SO_REUSEADDR: 서버를 껐다 바로 켤 때 "주소가 이미 사용 중" 에러 방지
    int optval = 1;
    if (setsockopt(m_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) < 0)
        throw std::runtime_error("setsocket() 실패");

    // 3) 논블로킹으로 전환 
    setNonBlocking(m_fd);

    // 4) bind :  
    // sockaddr_in 구조체 만듦(주소 담을)
	struct sockaddr_in	addr;
	std::memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
    // htonl : host to network long
    // htons : host to network short 
	// INADDR_ANY : 0이 기기의 모든 네트워크 인터페이스에서 오는 연결을 다 받겠다
	addr.sin_addr.s_addr = htonl(INADDR_ANY);			
	// 호스트->네트워크 바이트순서
	addr.sin_port = htons(static_cast<uint16_t>(port));
	// bind : "0.0.0.0:port"에 이 소켓을 묶는다.
	if (bind(m_fd, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0)
		throw std::runtime_error("bind() 실패");

	// 5) listen: 이제부터 접속 요청을 받겠다고 선언
	//SOMAXCONN의 의미
	//SOMAXCONN은 Socket Maximum Connections의 약자로, 
	//<sys/socket.h> 헤더 파일에 정의되어 있는 매크로 상수입니다.
	// 이 값을 listen()의 두 번째 인자로 넘겨주면, 운영체제(OS)에게 
	//"이 시스템(운영체제)이 허용하는 가장 큰 사이즈로 대기열(Backlog Queue)을 만들어줘!"라고 요청하는 것입니다.
	if (listen(m_fd, SOMAXCONN) < 0)
		throw std::runtime_error("listen() 실패");

	return m_fd;
}

void	Socket::setNonBlocking(int fd)
{
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
		throw std::runtime_error("fcntl(O_NONBLOCK) 실패");
}

int	Socket::getFd() const
{
	return m_fd;
}
