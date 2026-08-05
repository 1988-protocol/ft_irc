// 1. 헤더 상단 include 구문 아래에 전방선언 1줄 추가
class Client;

// 2. 기존 reply(...) 함수 선언 바로 아래에 1줄 추가
std::string buildMessage(const Client& client, const std::string& cmd, const std::string& target, const std::string& msg);
