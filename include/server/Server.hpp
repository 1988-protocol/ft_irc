#ifndef SERVER_HPP
#define SERVER_HPP

#include <stdexcept>
#include <string>
#include <map>

#include "server/Socket.hpp"
#include "server/PollManager.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"

class Server{
    
    private :
        Server();
        Server(Server const& other);
        Server& operator=(const Server& other);

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

        // Server Auth 관련 ────────────────────────────────────────────────────────
        Parser         m_parser;
        std::map<std::string, Client*> m_nicknames; // 닉네임 -> 클라이언트 포인터

        // Server cmds 관련────────────────────────────────────────────────────────

    public :
        //ocf
        Server(int port, const std::string &password);
        ~Server();
        
        // 메인 실행 루프
        void    run();

        static void signalHandler(int sig);

        // Server Auth 관련 ────────────────────────────────────────────────────────
        const std::string& getPassword() const;
        bool isNicknameInUse(const std::string& nickname) const;
        void registerNickname(const std::string& nickname, Client& client);
        void releaseNickname(const std::string& nickname);

        //Server cmds 관련────────────────────────────────────────────────────────

};

#endif

