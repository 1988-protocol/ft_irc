#include "common/Utils.hpp"
#include "client/Client.hpp"
#include <sstream>
#include <algorithm>
#include <cctype>

// 인스턴스화가 불필요한 class 대신 namespace를 사용하는 것도 구조적, 성능적으로 좋은 C++ 스타일이라고 함.
// 이건 익명 네임스페이스(unnamed namespace)라고 부릅니다. = C언어의 static 과 같은 기능.
// 다른 파일에서는 이 내용물을 볼 수 없음
namespace
{
    // 서버 식별 이름.
    // Parser의 reply() 헬퍼는 그와 무관하게 지금 바로 동작해야 하므로 상수로 둔다.
    // Server 실구현 이후 실제 호스트명이 필요해지면 이 지점만 교체하면 된다.
    const char* SERVER_NAME = "ircserv";
}

std::string getServerName()
{
    return SERVER_NAME;
}

std::string reply(int code, const std::string& target, const std::string& msg)
{
    // ":ircserv 431 * :No nickname given\r\n" 이와 같이 출력하기 위한 하드코딩
    // numeric은 RFC1459상 항상 3자리 문자열(예: "001", "461") — 앞을 '0'으로 채운다.
    std::ostringstream codeStream;
    if (code < 100)
        codeStream << '0';
    if (code < 10)
        codeStream << '0';
    codeStream << code;

    std::string line = ":";
    line += SERVER_NAME;
    line += ' ';
    line += codeStream.str();
    line += ' ';
    line += target;
    line += ' ';
    line += msg;
    line += "\r\n";
    return line;
}

// Channel command에서 Braodcasting Message 조합하기 위한 함수
std::string buildMessage(const Client& client, const std::string& cmd, const std::string& target, const std::string& msg)
{
    std::string line = ":" + client.getNickname() + "!" + client.getUsername() + "@" + client.getIp();
    line += " " + cmd;
    if (!target.empty())
        line += " " + target;
    if (!msg.empty())
        line += " :" + msg;
    line += "\r\n";
    return line;
}

// 명명된 네임스페이스
// 7.5  Nick의 헬퍼함수를 Util로 옮겼습니다.
// nick 검증은 코드 전체적으로 사용되어야 해서요.
namespace Utils
{
    std::vector<std::string> split(const std::string& s, char delim)
    {
        std::vector<std::string> result;
        std::string::size_type start = 0;
        std::string::size_type end = s.find(delim);

        while (end != std::string::npos)
        {
            // 연속된 delim은 빈 조각을 만들지 않고 하나의 구분자로 취급한다
            // (IRC 라인의 중복 공백 엣지 케이스 대응).
            if (end != start)
            {
                result.push_back(s.substr(start, end - start));
            }
            start = end + 1;
            end = s.find(delim, start);
        }
        if (start < s.size())
        {
            result.push_back(s.substr(start));
        }
        return result;
    }

    std::string trim(const std::string& s)
    {
        std::string::size_type start = s.find_first_not_of(' ');
        if (start == std::string::npos)
            return "";
        std::string::size_type end = s.find_last_not_of(' ');
        return s.substr(start, end - start + 1);
    }

    std::string toUpper(const std::string& s)
    {
        std::string result = s;
        // std::transform과 안전한 인라인 함수 safeToUpper를 사용하여 대문자로 변환합니다.
        std::transform(result.begin(), result.end(), result.begin(), safeToUpper);
        return result;
    }

    char toIRCLower(char c) // 주어진 문자를 IRC 프로토콜 2.2 규격에 맞춘 소문자로 변환.
    {
        if (c >= 'A' && c <= 'Z')
            return static_cast<char>(c + ('a' - 'A'));
        if (c == '[') return '{';
        if (c == ']') return '}';
        if (c == '\\') return '|';
        return c;
    }

    std::string toIRCLower(const std::string& s)
    {
        std::string result = s;
        for (std::string::size_type i = 0; i < result.size(); ++i)
            result[i] = toIRCLower(result[i]);
        return result;
    }

    bool isSameNickname(const std::string& n1, const std::string& n2)
    {
        if (n1.size() != n2.size())
            return false;
        for (std::string::size_type i = 0; i < n1.size(); ++i)
        {
            if (toIRCLower(n1[i]) != toIRCLower(n2[i]))
                return false;
        }
        return true;
    }

    namespace
    {
        bool isValidNicknameChar(char c, bool isFirst)
        {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
                return true;
            if (isFirst)
                return false; // 첫 글자는 알파벳만 허용 (RFC1459 2.3.1)
            if (c >= '0' && c <= '9')
                return true;
            static const std::string specials = "-[]\\`^{}_|";
            // rfc 1459에 의하면 _ , |는 불포함이었는데 irssi 에서 _를 기본적으로 추가하고 있어서 추가했습니다.
            // rfc 근거가 아니라 reference client의 근거로 추가했습니다.
            return specials.find(c) != std::string::npos;
        }
    }

    bool isValidNickname(const std::string& nickname)
    {
        if (nickname.empty() || nickname.size() > 9)
            return false;
        for (std::string::size_type i = 0; i < nickname.size(); ++i)
        {
            if (!isValidNicknameChar(nickname[i], i == 0))
                return false;
        }
        return true;
    }
}
