#ifndef SERVER_HPP
#define SERVER_HPP

#include <algorithm>
#include <stdexcept>
#include <string>
#include <map>

#include "server/Socket.hpp"
#include "server/PollManager.hpp"
#include "client/Client.hpp"
#include "parser/Parser.hpp"
#include "channel/Channel.hpp"

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
        void updateWriteEvents();

        // Server Auth 관련 ────────────────────────────────────────────────────────
        Parser         m_parser;

        // Server cmds 관련────────────────────────────────────────────────────────
        std::map<std::string, Channel*> m_channels;

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

        //Server cmds 관련────────────────────────────────────────────────────────
        Channel* getChannel(const std::string& channelName);
        const std::map<std::string, Channel*>& getChannels() const;
        void addChannel(const std::string& channelName, Channel* channel);
        void removeChannel(const std::string& channelName);
        Client* getClientByNick(const std::string& nickname);
        size_t getUserJoinedChannelCount(Client* client) const;
};

#endif

