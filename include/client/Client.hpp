#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

/*
 * Client
 * ------
 * 연결된 소켓 하나(= 접속한 유저 한 명)를 표현한다.
 * Network(나)가 생성/소멸하지만, Week 2부터 Parser가 등록 상태를,
 * Channel이 소속 정보를 이 객체에 붙여 참조하게 된다.
 *
 * ── Week 1에서 이 클래스의 존재 이유(가장 중요) ──
 * recv()는 데이터를 '조각'으로 준다. 지침의 nc 테스트처럼 'com','man','d\n'이
 * 따로 도착할 수 있다. 그래서 클라이언트마다 '입력 버퍼'를 두고 \n이 올 때까지
 * 모았다가, 완성된 한 줄만 뽑아 처리한다. 반대로 보낼 때도 send()가 한 번에
 * 다 못 보낼 수 있어 '출력 버퍼'에 남겨 다음 기회에 마저 보낸다.
 *
 * ── Week 2에서 여기에 붙을 필드(지금은 비워둠) ──
 *   nickname, username, realname (Parser 소유)
 *   registered/passOk 같은 등록 상태 플래그 (Parser가 set)
 *   소속 채널 참조 (Channel 담당과 방식 합의 후)
 */
class Client
{
private:
	int			_fd;		// 이 클라이언트의 소켓 fd
	std::string	_ip;		// 접속 IP(로그/prefix용)

	std::string	_inBuffer;	// 받은 바이트 중 아직 한 줄이 안 된 나머지
	std::string	_outBuffer;	// 보내야 하는데 아직 다 못 보낸 데이터

	bool		_markedForDeletion;	// 버퍼 다 비우면 끊어야 하는 상태

public:
	Client(int fd, const std::string &ip);
	~Client();

	int					getFd() const;
	const std::string	&getIp() const;

	// ── 입력 버퍼 ──
	// 받은 바이트를 뒤에 이어붙인다.
	void	appendToInBuffer(const std::string &data);
	// 완성된 한 줄(\n까지)이 있으면 out에 담고 버퍼에서 제거 후 true.
	// 아직 줄이 다 안 왔으면(부분 패킷) false.
	bool	extractLine(std::string &out);

	// ── 출력 버퍼 ──
	void				appendToOutBuffer(const std::string &data);
	bool				hasPendingOutput() const;	// 보낼 게 남았나?
	std::string			&getOutBuffer();			// Server가 send 후 앞부분을 지움

	// ── 종료 신호 ──
	void	markForDeletion();		// "이 클라 끊어" 표시(즉시 아님)
	bool	needsDisconnect() const;
};

#endif
