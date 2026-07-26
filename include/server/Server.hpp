#ifndef SERVER_HPP
#define SERVER_HPP

#include <stdexcept>
#include <string>
#include <map>

#include "server/Socket.hpp"
#include "server/PollManager.hpp"
#include "client/Client.hpp"

class Server{
    
    private :
        int             m_port;
        std::string     m_password;

        // 기본 생성자로 생성됨
        Socket          m_listener;
        PollManager     m_poll;
        std::map<int, Client*>  m_clients;

        static bool     m_running;

        // 연결 수명 관리
        void acceptNewClient();
        void receiveFromClient(int fd);
        void sendToClient(int fd);
        void disconnectClient(int fd);
        
        void setSignal();
        void handleLine(Client *client, const std::string &line);

    public :
        //ocf를 위한 것들. 구현에대해서는 합치면서 더 자세하게 봐야할 듯
        Server();
        Server(Server const& other);
        Server& operator=(const Server& other);

        Server(int port, const std::string &password);
        ~Server();
        
        // 메인 실행 루프
        void    run();

        static void signalHandler(int sig);
};

#endif

