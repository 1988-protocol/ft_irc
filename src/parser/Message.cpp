#include "parser/Message.hpp"
#include "common/Utils.hpp"

Message::Message()
    : m_hasTrailing(false)
{
}

const std::string& Message::getPrefix() const { return m_prefix; }
const std::string& Message::getCommand() const { return m_command; }
const std::vector<std::string>& Message::getParams() const { return m_params; }
const std::string& Message::getTrailing() const { return m_trailing; }
bool Message::hasTrailing() const { return m_hasTrailing; }

void Message::setPrefix(const std::string& prefix) { m_prefix = prefix; }
void Message::setCommand(const std::string& command) { m_command = command; }
void Message::addParam(const std::string& param) { m_params.push_back(param); }

void Message::setTrailing(const std::string& trailing)
{
    m_trailing = trailing;
    m_hasTrailing = true;
}

// RFC1459 2.3.1 문법(단, \r\n은 Network가 이미 제거했다고 가정):
//   message    = [ ":" prefix SPACE ] command [ params ] crlf
//   params     = *14( SPACE middle ) [ SPACE ":" trailing ]
//              =/ 14( SPACE middle ) [ SPACE [ ":" ] trailing ]
//
// 커맨드/파라미터 사이의 공백은 코드상 다중 공백이 와도 하나의 구분자로 취급한다
// (pre_plan.md Phase1 "중복 공백" 엣지 케이스) — Utils::split이 이를 처리한다.
// trailing은 " :" 마커 이후 끝까지를 공백 보존한 채로 그대로 가져간다(예: "hello   world").
// command는 여기서 대소문자를 정규화하지 않는다 — 대소문자 무시 비교는 Parser(디스패처)의
// 책임이다(Message는 원문을 그대로 보존하는 것이 파싱 계층의 역할).
Message Message::parse(const std::string& rawLine)
{
    Message msg;
    
    // 1. 전체 라인의 유효 구간(Trim 적용할 실제 경계)을 인덱스로만 탐색합니다.
    // 임시 문자열을 생성하지 않으므로 오버헤드가 0입니다.
    std::string::size_type start = rawLine.find_first_not_of(' ');
    if (start == std::string::npos)
        return msg; // 공백만 있는 줄은 즉시 빈 메시지 반환
        
    std::string::size_type end = rawLine.find_last_not_of(' ');

    // 2. Prefix 파싱 (첫 글자가 ':' 인지 경계 검사)
    if (rawLine[start] == ':')
    {
        // prefix는 첫 공백 전까지입니다.
        std::string::size_type sp = rawLine.find(' ', start);
        if (sp == std::string::npos || sp > end)
        {
            // command가 없는 비정상 메시지. prefix만 담아 반환
            msg.setPrefix(rawLine.substr(start + 1, end - start));
            return msg;
        }
        msg.setPrefix(rawLine.substr(start + 1, sp - (start + 1)));
        
        // 다음 파싱할 시작점을 공백 뒤의 유효한 문자로 이동
        start = rawLine.find_first_not_of(' ', sp + 1);
        if (start == std::string::npos || start > end)
            return msg;
    }

    // 3. Trailing 파싱 (유효 구간 내에서 " :" 마커 탐색)
    std::string::size_type trailingMarker = rawLine.find(" :", start);
    std::string::size_type middleEnd = end;
    std::string::size_type trailingStart = std::string::npos;
    bool foundTrailing = false;

    // 마커가 유효 범위 내에 있을 때만 처리
    if (trailingMarker != std::string::npos && trailingMarker < end)
    {
        middleEnd = trailingMarker - 1;       // " :" 직전 문자까지가 middle 영역
        trailingStart = trailingMarker + 2;   // " :" 직후 문자부터가 trailing 영역
        foundTrailing = true;
    }

    // 4. Middle 구간 토큰화
    if (start <= middleEnd)
    {
        // split에 필요한 부분만 최소한으로 substr합니다.
        std::string middlePart = rawLine.substr(start, middleEnd - start + 1);
        std::vector<std::string> tokens = Utils::split(middlePart, ' ');
        if (!tokens.empty())
        {
            msg.setCommand(tokens[0]);
            for (std::vector<std::string>::size_type i = 1; i < tokens.size(); ++i)
                msg.addParam(tokens[i]);
        }
    }

    // 5. Trailing 값 대입
    if (foundTrailing)
    {
        msg.setTrailing(rawLine.substr(trailingStart, end - trailingStart + 1));
    }

    return msg;
}
