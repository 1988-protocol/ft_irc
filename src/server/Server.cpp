#include "server/Server.hpp"

#include <iostream>
#include <csignal>          // sigaction, signal, sigemptyset
#include <cstring>          // memset
#include <cerrno>           // errno, EINTR
#include <cstddef>          // std::size_t
#include <vector>           // std::vector
#include <unistd.h>
#include <poll.h>
#include <netinet/in.h>     // sockaddr_in
#include <sys/socket.h>     // accept, recv, send
#include <arpa/inet.h>      // inet_ntoa

bool	Server::m_running = true;

// ────────────────────────────────────────────────────────
// 생성자/복사생성자/복사대입연산자/소멸자
// ────────────────────────────────────────────────────────

Server::Server(int port, const std::string &password)
    : m_port(port), m_password(password)
{
    // 1) 리스닝 소켓 생성
    int fd = m_listener.createListener(m_port);
    // 2) poll에 추가
    m_poll.add(fd);
    // 3) 시그널 설치
    setSignal();
	std::cout << "[server] 포트 " << m_port << " 에서 대기 시작" << std::endl;
}

Server::~Server()
{
    // 클라이언트 소켓만 정리, 리스닝 소켓은 리스닝 소켓 클래스가 해결
    for (std::map<int, Client*>::iterator it = m_clients.begin();
        it != m_clients.end(); ++it)
    {
        close(it->first);
        delete it->second;
    }
    m_clients.clear();

    // 채널 소멸자 호출
    for (std::map<std::string, Channel*>::iterator it = m_channels.begin();
        it != m_channels.end(); ++it)
    {
        delete it->second;
    }
    m_channels.clear();
}

// ────────────────────────────────────────────────────────
// 메인 루프
// ────────────────────────────────────────────────────────

void Server::run(){

    while(m_running)
    {
        // poll 대기
        if (m_poll.wait() < 0)
        {
            if(errno == EINTR)
                continue;
            break;
        }
        
        // 목록을 처음부터 끝까지 훑으며 이벤트 확인
        for(std::size_t i = 0; i < m_poll.size(); i++)
        {
            int fd = m_poll.getFd(i);
            short re = m_poll.getEvent(i);

            int listenFd = m_listener.getFd();
            if (re & (POLLERR | POLLHUP | POLLNVAL) && fd != listenFd)
            {
                disconnectClient(fd);
                --i;
                continue;
            }
            
            // 1) 읽을게 있음
            if(re & POLLIN)
            {
                // 읽기 이벤트 발생
                // 1-1) fd가 리스닝 소켓이면 accept, 아니면 recv
                if(fd == m_listener.getFd())
                {
                    // accept client
                    acceptNewClient();
                }
                else{
                    
                    // 1-2 ) 연결된 소켓에서 보낼 내용이 있다는 의미
                    receiveFromClient(fd); //데이터 읽기
                    // 읽는 중 끊겨서 사라졌으면 인덱스 보정 후 넘어감
                    if (m_clients.find(fd) == m_clients.end())
                    {
                        --i;
                        continue;
                    }
                }
            }
            // 2) 보낼 수 있는 상황이 됐음
            if(re & POLLOUT)
            {
                if(m_clients.find(fd) != m_clients.end())
                {
                    sendToClient(fd); //데이터 쓰기
                    // 보내는 중 끊겨서 사라졌으면 인덱스 보정 후 넘어감
                    if (m_clients.find(fd) == m_clients.end())
                    {
                        --i;
                        continue;
                    }
                }
            }
        }
        updateWriteEvents();
    }
    std::cout <<"\n[server] 종료합니다." << std::endl;
}

void Server::disconnectClient(int fd)
{
    std::map<int, Client*>::iterator    it = m_clients.find(fd);
    if(it == m_clients.end())
        return;
    
    std::cout << "[server] 연결 종료 (fd " << fd << ")" << std::endl;

    // Client 객체를 해제하기 전에, 이 포인터의 사본을 들고 있는 컨테이너를 모두 비운다.
    // 사본이 남는 곳은 채널마다 존재하는 m_members / m_invitedUsers 두 개뿐이고,
    // removeMember()가 내부에서 removeInvite()까지 연쇄 호출하므로 한 번만 부르면 된다.
    // (멤버가 아닌 채널에 호출해도 안전 — 초대만 받고 입장하지 않은 잔재까지 같이 정리된다)
    std::vector<std::string>    emptyChannels;

    for (std::map<std::string, Channel*>::iterator ch = m_channels.begin();
        ch != m_channels.end(); ++ch)
    {
        if (!ch->second)
            continue;
        ch->second->removeMember(it->second);
        // 멤버가 0명이 된 채널은 삭제한다.
        // 지금까지 이 규칙은 PART/KICK에만 있어서, QUIT·소켓 끊김·POLLERR 등으로
        // 마지막 멤버가 사라지면 빈 채널이 그대로 남았다.
        // disconnectClient는 그 모든 끊김 경로가 거쳐가는 지점이므로 여기서 함께 처리한다.
        if (ch->second->getMembers().empty())
            emptyChannels.push_back(ch->first);
    }

    for (std::size_t i = 0; i < emptyChannels.size(); ++i)
        removeChannel(emptyChannels[i]);

    m_poll.remove(fd);
    close(fd);
    delete it->second;
    m_clients.erase(it);
}

//함수 만들어야함 
void Server::acceptNewClient()
{
    struct sockaddr_in  client;
    socklen_t           client_len = sizeof(client);

    // poll()이 리스닝 소켓에 POLLIN을 준 뒤 accept를 정확히 한 번만 호출한다.
    // poll은 level-triggered라서 대기 중인 접속이 더 남아 있으면
    // 다음 루프에서 POLLIN이 다시 올라온다 -> 반복 accept로 fd를 독점할 이유가 없다.
    //client와 연결을 유지하는 새로운 socket을 생성합니다 (서버의 리스닝 소켓과는 별개)
    int clientFd = accept(m_listener.getFd(),
                                reinterpret_cast<struct sockaddr*>(&client), &client_len);
    // 실패하면 다음 poll을 기다린다.
    // (handshake 후 accept 전에 클라이언트가 RST를 보낸 경우 등)
    if (clientFd < 0)
        return;

    // accept 성공
    // 새로운 fd를 논블로킹으로 설정
    if(!Socket::setNonBlocking(clientFd))
    {
        close(clientFd);
        return;
    }

    // 새로운 클라이언트를 만들어야함.
    Client *new_client = NULL;
    try {
        std::string ip = inet_ntoa(client.sin_addr);
        new_client = new Client(clientFd, ip);
        m_clients[clientFd] = new_client;
        m_poll.add(clientFd);
        std::cout << "[server] 새 접속: " << ip << " (fd " << clientFd << ")" << std::endl;
    }
    catch (const std::exception &) {
        m_clients.erase(clientFd);
        delete new_client;
        close(clientFd);
        return;
    }
}


void Server::receiveFromClient(int fd)
{
    char buf[4096];
    int recv_len;

    recv_len = recv(fd, buf, sizeof(buf), 0);

    if (recv_len == 0)
    {
        // 상대가 정상적으로 연결을 닫음
        disconnectClient(fd);
        return;
    }

    // poll()이 POLLIN을 준 직후이므로 여기서의 실패는 진짜 오류로 본다.
    if (recv_len < 0)
    {
        disconnectClient(fd);
        return ;
    }

    std::map<int, Client*>::iterator it = m_clients.find(fd);
    if (it == m_clients.end())
        return;
    Client *client = it->second;
    client->appendToInBuffer(std::string(buf, recv_len));

    std::string line;
    while (client->extractLine(line))
        handleLine(client, line);
    
    if (!client->hasPendingOutput() && client->needsDisconnect())
        disconnectClient(fd);
}

// ────────────────────────────────────────────────────────
// 한줄 명렁 처리 : 파서로 넘어가는 부분
// ────────────────────────────────────────────────────────

void    Server::handleLine(Client *client, const std::string &line)
{
    std::cout << "[recv fd " << client->getFd() << "]" << line << std::endl;

    m_parser.process(*this, *client, line);
}

void Server::sendToClient(int fd)
{
    std::map<int, Client*>::iterator it = m_clients.find(fd);
    if (it == m_clients.end())
        return;
    Client  *client  = it->second;
    std::string &out = client->getOutBuffer();

    if (out.empty())
    {
        return;
    }
    // poll()이 POLLOUT을 준 직후이므로 여기서의 실패는 진짜 오류로 본다.
    ssize_t n = send(fd, out.c_str(), out.size(), 0);
    if(n < 0)
    {
        disconnectClient(fd);
        return;
    }

    out.erase(0, n);

    if(out.empty() && client->needsDisconnect())
    {
        disconnectClient(fd);
    }
}

void Server::updateWriteEvents()
{
    for (std::map<int, Client*>::iterator it = m_clients.begin();
        it != m_clients.end(); ++it)
    {
        m_poll.setWritable(it->first, it->second->hasPendingOutput());
    }
}

// ────────────────────────────────────────────────────────
// 시그널 처리
// ────────────────────────────────────────────────────────

void Server::setSignal()
{
    struct  sigaction   sa;
    std::memset(&sa, 0, sizeof(sa));
    sa.sa_handler = &Server::signalHandler;
    // 시그널 집합을 완저히 비우면서, 핸들러 함수가 실행되는 동안 다른 시그널을 막지 않겠다는 
    sigemptyset(&sa.sa_mask);
    // flag=0 ; 기본 동작
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, 0);
    sigaction(SIGTERM, &sa, 0);
    signal(SIGPIPE, SIG_IGN);
}

void Server::signalHandler(int sig)
{
    (void)sig;
    m_running = false;
}