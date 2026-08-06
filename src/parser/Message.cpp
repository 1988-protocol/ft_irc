#include "parser/Message.hpp"
#include "common/Utils.hpp"

Message::Message()
    : m_hasTrailing(false)
{
}

Message::Message(const Message& other)
    : m_prefix(other.m_prefix),
      m_command(other.m_command),
      m_params(other.m_params),
      m_trailing(other.m_trailing),
      m_hasTrailing(other.m_hasTrailing)
{
}

Message& Message::operator=(const Message& other)
{
    if (this != &other)
    {
        m_prefix = other.m_prefix;
        m_command = other.m_command;
        m_params = other.m_params;
        m_trailing = other.m_trailing;
        m_hasTrailing = other.m_hasTrailing;
    }
    return *this;
}

Message::~Message()
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
// <middle>   ::= <Any non-empty sequence of octets not including SPACE or NUL or CR or LF, the first of which may not be ':'>
// <trailing> ::= <Any sequence of octets not including NUL or CR or LF>
//
// 커맨드/파라미터 사이의 공백은 코드상 다중 공백이 와도 하나의 구분자로 취급한다
// (pre_plan.md Phase1 "중복 공백" 엣지 케이스) — Utils::split이 이를 처리한다.
// trailing은 " :" 마커 이후 끝까지를 공백 보존한 채로 그대로 가져간다(예: "hello   world").
// command는 여기서 대소문자를 정규화하지 않는다 — 대소문자 무시 비교는 Parser(디스패처)의
// 책임이다(Message는 원문을 그대로 보존하는 것이 파싱 계층의 역할).
Message Message::parse(const std::string& rawLine)
{
    Message msg;

    // [보완] 줄 끝에 \r이 남아있다면 방어적으로 제거하여 파서 자체의 독립성 확보
    std::string line = rawLine;
    if (!line.empty() && line[line.size() - 1] == '\r')
    {
        line.erase(line.size() - 1);
    }

    // NUL 문자 검증 (Note 4) - 보안 취약점 차단 및 Fail-Fast
    // 네트워크 단에서 처리하게 된 경우 여기 if문 제거
    if (line.find('\0') != std::string::npos)
    {
        return msg;
    }
    
    // 1. 전체 라인의 유효 구간(선행 공백 제외)을 찾습니다.
    std::string::size_type i = line.find_first_not_of(' ');
    if (i == std::string::npos)
        return msg; // 공백만 있는 줄은 즉시 빈 메시지 반환

    // 2. Prefix 파싱 (첫 글자가 ':' 인지 경계 검사)
    if (line[i] == ':')
    {
        std::string::size_type next_space = line.find(' ', i);
        if (next_space == std::string::npos)
        {
            // command가 없는 비정상 메시지. prefix만 담아 반환
            msg.setPrefix(line.substr(i + 1));
            return msg;
        }
        msg.setPrefix(line.substr(i + 1, next_space - (i + 1)));
        
        // 다음 파싱할 시작점을 공백 뒤의 유효한 문자로 이동
        i = line.find_first_not_of(' ', next_space);
    }
    if (i == std::string::npos)
        return msg;

    // 3. Command 파싱
    std::string::size_type cmd_end = line.find(' ', i);
    if (cmd_end == std::string::npos)
    {
        msg.setCommand(line.substr(i));
        return msg;
    }
    msg.setCommand(line.substr(i, cmd_end - i));
    i = line.find_first_not_of(' ', cmd_end);

    // 4. Params 파싱 루프 (최대 14개 수집)
    while (i != std::string::npos && msg.getParams().size() < 14)
    {
        // 공백 뒤에 바로 ':'이 오면 trailing 마커입니다.
        // 콜론 뒤의 모든 문자를 trailing으로 저장 (빈 문자열 ":" 만 전송된 경우도 hasTrailing() == true 처리)
        if (line[i] == ':')
        {
            msg.setTrailing(line.substr(i + 1));
            return msg;
        }
        std::string::size_type param_end = line.find(' ', i);
        if (param_end == std::string::npos)
        {
            msg.addParam(line.substr(i));
            return msg;
        }
        msg.addParam(line.substr(i, param_end - i));
        i = line.find_first_not_of(' ', param_end);
    }

    // 5. 15번째 파라미터 (자동 Trailing) 처리 (RFC 1459 2.3.1 규격)
    if (i != std::string::npos)
    {
        if (line[i] == ':')
        {
            msg.setTrailing(line.substr(i + 1));
        }
        else
        {
            msg.setTrailing(line.substr(i));
        }
    }

    return msg;
}


// 예시문
// "PRIVMSG #lobby :Hello World! How are you?"
// (COMMAND PARAM) TRAILING
// MIDDLE

// ":Alice NICK Bob"
// PREFIX (COMMAND PARAM)
//              MIDDLE


