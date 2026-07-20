#ifndef SERVER_HPP
#define SERVER_HPP

#include <string>
#include <map>

#include "server/Socket.hpp"
#include "server/PollManager.hpp"
#include "client/Client.hpp"

/*
 * Server
 * ------
 * 전체를 지휘하는 오케스트레이터. Week 1의 목표는 딱 여기까지다:
 *   "단일 poll 루프 + accept + 라인 에코"
 *
 * 소유물:
 *   - Socket       : 리스닝 창구
 *   - PollManager  : 단 하나의 감시 목록
 *   - _clients     : fd -> Client* (접속한 모든 유저)
 *
 * Week 2에서 바뀔 단 한 곳:
 *   handleLine() 안의 '에코' 코드가 'Parser::parse(line)' 호출로 교체된다.
 *   나머지 뼈대(루프/버퍼/accept/disconnect)는 그대로 재사용된다.
 */
class Server
{
private:
	int							_port;
	std::string					_password;	// Week1 에코에선 미사용, Parser 단계서 인증에 사용

	Socket						_listener;	// 리스닝 소켓
	PollManager					_poll;		// 단일 poll 관리자
	std::map<int, Client*>		_clients;	// fd -> Client

	static bool					_running;	// 시그널로 뒤집히는 종료 플래그

	// ── 연결 수명 관리 ──
	void	acceptNewClient();					// 새 접속 받기
	void	receiveFromClient(int fd);			// recv -> 버퍼 -> 줄 처리
	void	sendToClient(int fd);				// 출력 버퍼 flush
	void	disconnectClient(int fd);			// 끊고 정리

	// ── 한 줄(명령)이 완성됐을 때 처리 ──
	// ★ Week 1: 여기서 그냥 에코한다.
	// ★ Week 2: 이 함수 내부가 Parser 호출로 교체된다.
	void	handleLine(Client *client, const std::string &line);

public:
	Server(int port, const std::string &password);
	~Server();

	void	run();	// 메인 루프

	static void	signalHandler(int sig);	// Ctrl-C 등에서 _running을 false로
};

#endif
