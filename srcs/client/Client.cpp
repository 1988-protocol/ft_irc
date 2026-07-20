#include "client/Client.hpp"

Client::Client(int fd, const std::string &ip)
	: _fd(fd),
	  _ip(ip),
	  _inBuffer(""),
	  _outBuffer(""),
	  _markedForDeletion(false)
{
}

Client::~Client()
{
}

int					Client::getFd() const	{ return _fd; }
const std::string	&Client::getIp() const	{ return _ip; }

// ── 입력 버퍼 ──────────────────────────────────────────────
void	Client::appendToInBuffer(const std::string &data)
{
	// 받은 조각을 그냥 뒤에 이어붙인다. 아직 한 줄이 아닐 수도 있다.
	_inBuffer += data;
}

/*
 * extractLine
 * -----------
 * 부분 패킷 재조립의 핵심. 버퍼에서 '\n'을 찾는다.
 *   - 없으면: 아직 명령이 다 안 왔다 -> false (다음 recv를 기다림)
 *   - 있으면: \n 앞까지를 한 줄로 잘라 out에 담고, 그 줄을 버퍼에서 제거.
 * '\r\n'으로 끝나는 경우 뒤의 '\r'도 떼어낸다(IRC 표준은 CRLF).
 *
 * 예) 버퍼가 "command\r\nNEXT" 이면 -> out="command", 버퍼엔 "NEXT" 남음.
 */
bool	Client::extractLine(std::string &out)
{
	std::string::size_type	pos = _inBuffer.find('\n');
	if (pos == std::string::npos)
		return false;	// 완성된 줄 없음 = 부분 패킷 상태

	out = _inBuffer.substr(0, pos);		// \n 앞까지
	_inBuffer.erase(0, pos + 1);		// 처리한 줄 + \n 제거

	// CRLF의 '\r' 제거
	if (!out.empty() && out[out.size() - 1] == '\r')
		out.erase(out.size() - 1);

	return true;
}

// ── 출력 버퍼 ──────────────────────────────────────────────
void	Client::appendToOutBuffer(const std::string &data)
{
	_outBuffer += data;
}

bool	Client::hasPendingOutput() const
{
	return !_outBuffer.empty();
}

std::string	&Client::getOutBuffer()
{
	// Server가 send() 후 보낸 만큼 앞에서 지우기 위해 참조로 돌려준다.
	return _outBuffer;
}

// ── 종료 신호 ──────────────────────────────────────────────
void	Client::markForDeletion()			{ _markedForDeletion = true; }
bool	Client::needsDisconnect() const		{ return _markedForDeletion; }
