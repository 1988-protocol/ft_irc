#include "server/PollManager.hpp"

PollManager::PollManager()
{
}

PollManager::~PollManager()
{
}

/*
 * add
 * ---
 * 감시 목록에 fd를 추가한다. 처음엔 POLLIN(읽을 게 생기면 알려줘)만 켠다.
 * revents는 0으로 초기화(아직 아무 일도 안 일어남).
 */
void	PollManager::add(int fd)
{
	struct pollfd	pfd;
	pfd.fd = fd;
	pfd.events = POLLIN;
	pfd.revents = 0;
	_pfds.push_back(pfd);
}

/*
 * remove
 * ------
 * 목록에서 해당 fd를 찾아 제거(연결 종료 시).
 */
void	PollManager::remove(int fd)
{
	for (std::vector<struct pollfd>::iterator it = _pfds.begin();
		 it != _pfds.end(); ++it)
	{
		if (it->fd == fd)
		{
			_pfds.erase(it);
			return;
		}
	}
}

/*
 * setWritable
 * -----------
 * 특정 fd의 POLLOUT(쓰기 감시)을 켜고 끈다.
 *   on=true  -> events에 POLLOUT 비트를 추가 (|=)
 *   on=false -> POLLOUT 비트를 제거      (&= ~)
 * 이게 "보낼 게 있을 때만 쓰기 감시"의 핵심.
 */
void	PollManager::setWritable(int fd, bool on)
{
	for (std::vector<struct pollfd>::iterator it = _pfds.begin();
		 it != _pfds.end(); ++it)
	{
		if (it->fd == fd)
		{
			if (on)
				it->events |= POLLOUT;
			else
				it->events &= ~POLLOUT;
			return;
		}
	}
}

/*
 * wait
 * ----
 * 단 하나의 poll() 호출. 목록의 어떤 fd에서든 이벤트가 생길 때까지 잠든다.
 * 세 번째 인자 -1 = 타임아웃 없음(무한 대기). 그래서 할 일 없으면 CPU 0%.
 * &_pfds[0]은 벡터의 내부 배열 시작 주소(연속 메모리라 poll에 그대로 넘길 수 있음).
 */
int	PollManager::wait()
{
	return poll(&_pfds[0], _pfds.size(), -1);
}

// ── 결과 순회용 접근자 ──
std::size_t	PollManager::size() const				{ return _pfds.size(); }
int			PollManager::fdAt(std::size_t i) const		{ return _pfds[i].fd; }
short		PollManager::reventsAt(std::size_t i) const	{ return _pfds[i].revents; }
