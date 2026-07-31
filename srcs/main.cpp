#include "server/Server.hpp"

#include <cstdlib>
#include <iostream> // strtol
#include <string>

static bool parsePort(const std::string &arg, int &port)
{
    //arg 비었을 때
    if(arg.empty())
        return false;
    // arg 숫자 아닐 때
    for(std::string::size_type i = 0; i < arg.size(); ++ i)
        if (arg[i] < '0' || arg[i] > '9')
            return false;
    
    // 포트 범위 검증
    char    *end = 0;
    long    v = std::strtol(arg.c_str(), &end, 10);
    if  (*end != '\0' || v < 1 || v > 65535)
        return false;
    port = static_cast<int>(v);
    return true;
}

int main(int argc, const char* argv[]) {

    if (argc != 3)
    {
        std::cerr << "사용법: " << argv[0] << " <port> <password>" << std::endl;
        return 1;
    }

    // 포트 연결, 비번 연결
    int port;
    if (!parsePort(argv[1], port))
    {
        std::cerr << "에러: port는 1~65535 사이 정수여야 합니다." << std::endl;
        return 1;
    }

    std::string password(argv[2]);
    if (password.empty())
    {
        std::cerr << "에러: password는 비어 있으면 안 됩니다." << std::endl;
    }

    try
    {
        Server  server(port, password);
        server.run();
    }
    catch(const std::exception& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }    
    return 0;
}