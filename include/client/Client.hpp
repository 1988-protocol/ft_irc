#ifndef CLIENT_HPP
#define CLIENT_HPP

#include <string>

class Client {
    private:
        int             m_fd;
        std::string     m_ip;

        std::string     m_inBuffer;
        std::string     m_outBuffer;

        bool            m_markedForDeletion;

    public:
        // ocf
        Client();
        Client(Client const &other);
        Client &operator=(Client const &other);

        Client(int fd, std::string ip);
        ~Client();

        int     getFd() const;
        const std::string &getIp() const;

        void    appendToInBuffer(const std::string &data);
        bool    extractLine(std::string &out);

        void            appendToOutBuffer(const std::string &data);
        bool            hasPendingOutput() const;
        std::string     &getOutBuffer();

        void            markForDeletion();
        bool            needsDisconnect() const;
};

#endif // CLIENT_HPP