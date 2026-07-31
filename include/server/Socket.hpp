#ifndef SOCKET_HPP
#define SOCKET_HPP

// 리스닝 소켓 하나를 만드는 일만 책임짐
// Server가 이걸 통해 창구를 연다.

class Socket 
{
    private:
        int m_fd;
    public:
        Socket(Socket const &other);
        Socket &operator=(Socket const &other);
        
        Socket();
        ~Socket();

        int createListener(int port);
        static void setNonBlocking(int fd);

        int getFd() const;
};

#endif
