#include "client/Client.hpp"

// ────────────────────────────────────────────────────────
// ocf
// ────────────────────────────────────────────────────────

Client::Client() : m_fd(-1), m_ip(""), m_inBuffer(""), m_outBuffer(""), m_markedForDeletion(false) {}

Client::Client(int fd, std::string ip) : m_fd(fd), m_ip(ip), m_inBuffer(""), m_outBuffer(""), m_markedForDeletion(false) {}

Client::Client(const Client& other)
{
    *this = other;
}

Client& Client::operator=(const Client& other)
{
    if (this != &other)
    {
        this->m_fd = other.m_fd;
        this->m_ip = other.m_ip;
        this->m_inBuffer = other.m_inBuffer;
        this->m_outBuffer = other.m_outBuffer;
        this->m_markedForDeletion = other.m_markedForDeletion;
    }
    return *this;
}

Client::~Client() {}

// ────────────────────────────────────────────────────────
// 입력 버퍼
// ────────────────────────────────────────────────────────

void    Client::appendToInBuffer(const std::string &data)
{
    m_inBuffer += data;
}

bool    Client::extractLine(std::string &out)
{
    std::string::size_type pos = m_inBuffer.find('\n');
    if (pos == std::string::npos)
        return false;
    
    out = m_inBuffer.substr(0, pos);
    m_inBuffer.erase(0, pos + 1);

    if(!out.empty() && out[out.size() - 1] == '\r')
        out.erase(out.size() - 1);
    
    return true;
}

// ────────────────────────────────────────────────────────
// 출력
// ────────────────────────────────────────────────────────

void    Client::appendToOutBuffer(const std::string &data)
{
    m_outBuffer += data;
}

bool    Client::hasPendingOutput() const
{
    return !m_outBuffer.empty();
}

std::string &Client::getOutBuffer()
{
	// Server가 send() 후 보낸 만큼 앞에서 지우기 위해 참조로 돌려준다.
    return m_outBuffer;
}

// ────────────────────────────────────────────────────────
// 종료
// ────────────────────────────────────────────────────────

void    Client::markForDeletion() 
{
    m_markedForDeletion = true;
}

bool    Client::needsDisconnect() const
{
    return m_markedForDeletion;
}

// ────────────────────────────────────────────────────────

int Client::getFd() const
{
    return m_fd;
}

const std::string &Client::getIp() const
{
    return m_ip;
}

