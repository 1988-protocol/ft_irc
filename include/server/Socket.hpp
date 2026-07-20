#ifndef SOCKET_HPP
#define SOCKET_HPP

/*
 * Socket
 * ------
 * "리스닝 소켓"(손님을 받는 창구) 하나를 만드는 일만 책임지는 작은 클래스다.
 * 네트워크 담당(나)의 소유물이며, Server가 이걸 이용해 창구를 연다.
 *
 * 왜 따로 뺐나?
 *   socket() -> setsockopt() -> 논블로킹 -> bind() -> listen() 이라는
 *   "창구 여는 절차"를 한 군데 모아두면, Server 코드가 깔끔해지고
 *   나중에 IPv6나 옵션 변경도 여기만 고치면 된다.
 */
class Socket
{
private:
	int	_fd;	// 리스닝 소켓의 파일 디스크립터(-1이면 아직 안 열림)

public:
	Socket();
	~Socket();	// 소멸 시 열린 소켓을 close() 한다

	// 주어진 포트로 리스닝 소켓을 만들고 그 fd를 반환한다.
	// 실패하면 std::runtime_error 예외를 던진다(서버는 못 뜨는 게 정상).
	int		createListener(int port);

	// 아무 fd나 "논블로킹 모드"로 바꾼다.
	// 지침이 허용한 유일한 형태: fcntl(fd, F_SETFL, O_NONBLOCK)
	// static이라 객체 없이도 Server가 새 클라 fd에 바로 쓸 수 있다.
	static void	setNonBlocking(int fd);

	int		getFd() const;	// 리스닝 fd 조회
};

#endif
