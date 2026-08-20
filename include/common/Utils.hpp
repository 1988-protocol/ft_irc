#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <cctype>

// buildMessage 인자를 위한 전방선언
class Client;

// numeric reply
// ":<server> <code> <target> <msg>\r\n" 형태의 완성된 응답 줄을 만든다.
// code가 3자리 미만이면 앞을 '0'으로 채운다(RFC1459: numeric은 항상 3자리 문자열).
// msg에 커맨드명이 필요한 코드(예: 461 "NICK :Not enough parameters")는
// 호출부(각 커맨드 클래스)가 이미 그 형태로 msg를 만들어서 넘긴다 — reply()는 포맷팅만 담당.
std::string reply(int code, const std::string& target, const std::string& msg);

// Channel commands에서 Braodcasting Message를 build 해주는 함수
std::string buildMessage(const Client& client, const std::string& cmd, const std::string& target, const std::string& msg = "");

std::string getServerName();

namespace Utils
{
    // 하나 이상 연속된 delim을 하나의 구분자로 취급해 분리한다(중복 공백 대응).
    // 빈 조각은 결과에 포함하지 않는다.
    std::vector<std::string> split(const std::string& s, char delim);

    // 비ASCII 문자 입력 시 std::toupper의 정의되지 않은 동작(UB)을 방지하기 위한 안전한 대문자 변환 함수.
    inline char safeToUpper(char c)
    {
        return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    }

    // ASCII 알파벳만 대문자로 변환한다(커맨드명 대소문자 무시 비교용).
    std::string toUpper(const std::string& s);

    // RFC1459 IRC 대소문자 변환 ('A'-'Z' -> 'a'-'z', '[' -> '{', ']' -> '}', '\' -> '|')
    char toIRCLower(char c);
    // 문자열 전체를 IRC 정규화(lowercase) — m_nicknames 맵 키 정규화용.
    std::string toIRCLower(const std::string& s);

    // RFC1459 규격에 따라 두 닉네임이 동등한지 비교한다 (대소문자 및 IRC 특수문자 동등 취급).
    bool isSameNickname(const std::string& n1, const std::string& n2);

    // RFC1459 규격에 따른 닉네임 유효성 검사.
    bool isValidNickname(const std::string& nickname);
}

#endif

