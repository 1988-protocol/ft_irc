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

// 서버 식별 이름을 반환한다. Server 클래스가 아직 실구현되지 않은 상태(Network Phase1 몫)라
// 임시 상수를 감싸 반환하는 함수로 두었다 — 실제 호스트명이 필요해지면 이 함수 내부만 바뀌면 된다.
std::string getServerName();

// Message::parse()와 각 커맨드의 파라미터 검증에서 공통으로 쓰는 문자열 유틸리티.
// C++98이라 std::to_string이 없으므로 숫자<->문자열 변환도 필요하면 여기 추가한다.
namespace Utils
{
    // 하나 이상 연속된 delim을 하나의 구분자로 취급해 분리한다(중복 공백 대응).
    // 빈 조각은 결과에 포함하지 않는다.
    std::vector<std::string> split(const std::string& s, char delim);

    // 문자열 앞뒤의 공백류(space)만 제거한다. 내부 공백은 보존한다.
    std::string trim(const std::string& s);

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

