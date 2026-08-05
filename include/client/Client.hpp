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

        bool            m_registered;
        bool            m_hasCorrectPassword;
        std::string     m_nickname;
        std::string     m_username;
        std::string     m_realname;
        std::string     m_hostname;
        std::string     m_servername;

    public:
        // ocf
        Client();
        Client(Client const &other);
        Client &operator=(Client const &other);

        Client(int fd, std::string ip);
        ~Client();

        void    appendToInBuffer(const std::string &data);
        bool    extractLine(std::string &out);

        bool            appendToOutBuffer(const std::string &data);
        bool            hasPendingOutput() const;
        std::string     &getOutBuffer();

        void            markForDeletion();
        bool            needsDisconnect() const;

        //getter
        int           getFd() const;
        const std::string   &getIp() const;

        bool                isRegistered() const;
        bool                hasCorrectPassword() const;
        const std::string   &getNickname() const;
        const std::string   &getUsername() const;

        //setter
        void            setNickname(const std::string &nickname);
        void            setUsername(const std::string &username);
        void            setRegistered(bool registered);
        void            setHasCorrectPassword(bool hasCorrectPassword);
};

#endif // CLIENT_HPP