#ifndef POLLMANAGER_HPP
#define POLLMANAGER_HPP

#include <vector>
#include <cstddef>  // std::size_t
#include <poll.h>   //pollfd poll

// poll 배열에 내용추가, 

class PollManager
{
    private:
        std::vector<struct pollfd> m_pfds;

    public:
        PollManager();
        ~PollManager();

        // 목록에 fd추가, 기본적으로 읽기 검사만 한다.
        void    add(int fd);

        // 목록에서 fd 제거 (연결이 끊어졌을 때)
        void    remove(int fd);

        // 해당 fd의 POLLOUT(쓰기 감시)를 키거나 끈다.
        // on=true : 보낼 데이터가 생겼으니 "보낼 수 있게 되면 알려줘"
        // on=false : 다 보냈으니 더 이상 알림 필요가 없음
        void    setWritable(int fd, bool on);

        int    wait();
        
        int             getFd(std::size_t index) const;
        short             getEvent(std::size_t index) const;
        std::size_t     size()  const;
};

#endif