#include "server/Server.hpp"

#include <iostream>
#include <cstdlib>	// strtol
#include <string>

/*
 * 실행: ./ircserv <port> <password>
 *
 * Week 1 목표: 이 프로그램을 실행하면 poll 루프가 돌고, nc로 접속하면
 * 보낸 줄이 "echo: ..." 로 되돌아온다. 여러 명이 동시에 붙어도 안 막히고,
 * 명령을 조각내 보내도(com/man/d) 한 줄로 합쳐 처리된다.
 */

// 포트 문자열을 검사하고 정수로 변환. 1~65535 범위만 허용.
static bool	parsePort(const std::string &arg, int &port)
{
	if (arg.empty())
		return false;
	for (std::string::size_type i = 0; i < arg.size(); ++i)
		if (arg[i] < '0' || arg[i] > '9')
			return false;

	char	*end = 0;
	long	v = std::strtol(arg.c_str(), &end, 10);
	if (*end != '\0' || v < 1 || v > 65535)
		return false;
	port = static_cast<int>(v);
	return true;
}

int	main(int argc, char **argv)
{
	// 인자 개수 확인.
	if (argc != 3)
	{
		std::cerr << "사용법: ./ircserv <port> <password>" << std::endl;
		return 1;
	}

	int	port;
	if (!parsePort(argv[1], port))
	{
		std::cerr << "에러: port는 1~65535 사이 정수여야 합니다." << std::endl;
		return 1;
	}

	std::string	password(argv[2]);
	if (password.empty())
	{
		std::cerr << "에러: password는 비어 있으면 안 됩니다." << std::endl;
		return 1;
	}

	// 서버 생성/실행을 try로 감싼다.
	// bind 실패 같은 치명적 오류가 나도 크래시 대신 메시지 출력 후 종료.
	try
	{
		Server	server(port, password);
		server.run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "치명적 오류: " << e.what() << std::endl;
		return 1;
	}
	return 0;
}
