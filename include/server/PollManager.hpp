#ifndef POLLMANAGER_HPP
#define POLLMANAGER_HPP

#include <vector>
#include <cstddef>	// std::size_t
#include <poll.h>

/*
 * PollManager
 * -----------
 * "감시 목록"을 관리하는 클래스. 지침이 요구하는 '단일 poll()'의 실체다.
 *
 * 비유: 접수원이 들고 있는 '지켜볼 창구 명단'. 여기에 fd를 넣고 빼며,
 * wait()를 부르면 "이 중에 무슨 일이 생겼나" 한 번에 확인한다.
 *
 * 핵심 개념 2가지:
 *   - events  : "내가 이 fd에서 무엇을 기다리는가" (POLLIN=읽기, POLLOUT=쓰기)
 *   - revents : "실제로 무슨 일이 일어났는가" (poll이 채워서 돌려줌)
 *
 * POLLOUT(쓰기 가능)은 "보낼 데이터가 있을 때만" 켠다. 항상 켜두면
 * poll이 쉴 새 없이 깨어나 CPU를 낭비하기 때문이다. -> setWritable()
 */
class PollManager
{
private:
	std::vector<struct pollfd>	_pfds;	// 감시할 fd 목록(단 하나의 poll 대상)

public:
	PollManager();
	~PollManager();

	// 목록에 fd 추가. 기본으로 POLLIN(읽기)만 감시한다.
	void	add(int fd);

	// 목록에서 fd 제거(연결이 끊겼을 때).
	void	remove(int fd);

	// 해당 fd의 POLLOUT(쓰기 감시)을 켜거나 끈다.
	//   on=true  : 보낼 데이터가 생겼으니 "보낼 수 있게 되면 알려줘"
	//   on=false : 다 보냈으니 더 이상 알림 필요 없음
	void	setWritable(int fd, bool on);

	// 단 하나의 poll() 호출. 이벤트가 생길 때까지 잠든다(timeout -1).
	// 반환값: 이벤트가 발생한 fd 개수(음수면 에러).
	int		wait();

	// 아래 3개는 wait() 후 결과를 순회하기 위한 접근자.
	std::size_t	size() const;				// 목록 크기
	int			fdAt(std::size_t i) const;		// i번째 fd
	short		reventsAt(std::size_t i) const;	// i번째 fd에서 실제로 일어난 이벤트
};

#endif
