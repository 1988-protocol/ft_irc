#include "server/PollManager.hpp"

#include <stdexcept>	// std::runtime_error

// ────────────────────────────────────────────────────────
// ocf
// ────────────────────────────────────────────────────────

PollManager::PollManager()
{}

PollManager::~PollManager()
{}

// ────────────────────────────────────────────────────────
// add, remove
// ────────────────────────────────────────────────────────

void    PollManager::add(int fd)
{
    struct pollfd   pfd;
    pfd.fd = fd;
    pfd.events = POLLIN; // 읽을게 생기면 알려달라는뜻
    pfd.revents = 0;
    m_pfds.push_back(pfd);
}

void    PollManager::remove(int fd)
{
    for(std::vector<struct pollfd>::iterator it = m_pfds.begin();
        it != m_pfds.end(); it++)
        {
            if (it->fd == fd)
            {
                m_pfds.erase(it);
                return ;
            }
        }
}

void    PollManager::setWritable(int fd, bool on)
{
    for(std::vector<struct pollfd>::iterator it = m_pfds.begin();
            it != m_pfds.end(); ++it)
    {
        if (it->fd == fd)
        {
            if(on)
                m_pfds[fd].events |= POLLOUT;
            else
                m_pfds[fd].events &= ~POLLOUT;
            return;
        }
    }
}

int    PollManager::wait()
{
    // -1 : 무한 대기
    // 이벤트가 들어올 때까지
    return (poll(&m_pfds[0], m_pfds.size(),-1));
}


// ────────────────────────────────────────────────────────
// 단순 조회용 메서드
// ────────────────────────────────────────────────────────

int PollManager::getFd(std::size_t index) const
{
    return (m_pfds[index].fd);
}

short PollManager::getEvent(std::size_t index) const
{
    return (m_pfds[index].revents);
}

std::size_t PollManager::size() const
{
    return (m_pfds.size());
}
