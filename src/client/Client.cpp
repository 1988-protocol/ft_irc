#include "client/Client.hpp"

// ────────────────────────────────────────────────────────
// ocf
// ────────────────────────────────────────────────────────

Client::Client() : m_fd(-1), m_ip(""), m_inBuffer(""), m_outBuffer(""),
                    m_markedForDeletion(false), m_registered(false), m_hasCorrectPassword(false),
                    m_nickname(""), m_username(""), m_hostname(""), m_servername("") {}

Client::Client(int fd, std::string ip) : m_fd(fd), m_ip(ip), m_inBuffer(""), m_outBuffer(""),
                            m_markedForDeletion(false), m_registered(false), m_hasCorrectPassword(false),
                            m_nickname(""), m_username(""), m_hostname(""), m_servername("") {}

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

        this->m_registered = other.m_registered;
        this->m_hasCorrectPassword = other.m_hasCorrectPassword;
        this->m_nickname = other.m_nickname;
        this->m_username = other.m_username;
        this->m_hostname = other.m_hostname;
        this->m_servername = other.m_servername;
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
    {
        if (m_inBuffer.size() > 512)
            markForDeletion();
        return false;
    }
    if (pos > 511)
    {
        markForDeletion();
        return false;
    }
    out = m_inBuffer.substr(0, pos);
    m_inBuffer.erase(0, pos + 1);

    if(!out.empty() && out[out.size() - 1] == '\r')
        out.erase(out.size() - 1);
    
    return true;
}

// ────────────────────────────────────────────────────────
// 출력
// ────────────────────────────────────────────────────────

namespace
{
    const std::string::size_type kMaxOutBufferSize = 64 * 1024;
    // RFC 1459 2.3: CRLF 포함 512바이트 → 본문 510바이트.
    const std::string::size_type kMaxLineBody = 510;
}

bool    Client::appendToOutBuffer(const std::string &data)
{
    std::string line = data;
    if (line.size() > kMaxLineBody + 2)     // 510 + CRLF = 512
    {
        line.erase(kMaxLineBody);
        line += "\r\n";
    }

    if (m_outBuffer.size() > kMaxOutBufferSize
        || line.size() > kMaxOutBufferSize - m_outBuffer.size())
    {
        if (!m_markedForDeletion)
        {
            markForDeletion();
            m_outBuffer += "ERROR :Closing Link: " + m_nickname
                         + "[" + m_ip + "] (SendQ exceeded)\r\n";
        }
        return false;
    }

    m_outBuffer += line;
    return true;
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
// getter
// ────────────────────────────────────────────────────────

int Client::getFd() const             {return m_fd;}

const std::string &Client::getIp() const    {return m_ip;}

bool Client::isRegistered() const           {return m_registered;}

bool Client::hasCorrectPassword() const     {return m_hasCorrectPassword;}

const std::string &Client::getNickname() const   {return m_nickname;}

const std::string &Client::getUsername() const   {return m_username;}

const std::string &Client::getHostname() const   {return m_hostname;}

// ────────────────────────────────────────────────────────
// setter
// ────────────────────────────────────────────────────────

void Client::setNickname(const std::string &nickname) {m_nickname = nickname;}

void Client::setUsername(const std::string &username) {m_username = username;}

void Client::setRegistered(bool registered) {m_registered = registered;}

void Client::setHasCorrectPassword(bool hasCorrectPassword) {m_hasCorrectPassword = hasCorrectPassword;}


