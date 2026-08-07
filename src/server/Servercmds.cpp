#include "server/Server.hpp"
#include "channel/Channel.hpp"
#include "common/Utils.hpp"

// NOTE (채널 담당자 참고):
// RFC 2812 Section 1.3 규격상 채널명은 대소문자를 구분하지 않습니다 (Case-insensitive).
// 현재 구현은 std::map::find(channelName)으로 정확히 일치하는 문자열만 찾기 때문에,
// 유저 A가 '/join #Chat'으로 채널을 생성한 후 유저 B가 '/join #chat'으로 입장하면
// 서로 다른 2개의 채널로 격리되는 분리(Split) 버그가 발생할 수 있습니다.
// 따라서 m_channels 맵에 채널을 등록(addChannel), 삭제(removeChannel), 조회(getChannel)할 때
// 닉네임과 마찬가지로 Utils::toIRCLower(channelName)을 적용하여 정규화 키로 관리하는 것을 권장합니다.
Channel* Server::getChannel(const std::string& channelName)
{
    std::map<std::string, Channel*>::iterator it = m_channels.find(channelName);
    if (it != m_channels.end())
        return it->second;
    return NULL;
}

void Server::addChannel(const std::string& channelName, Channel* channel)
{
    m_channels[channelName] = channel;
}

void Server::removeChannel(const std::string& channelName)
{
    std::map<std::string, Channel*>::iterator it = m_channels.find(channelName);
    if (it != m_channels.end())
    {
        // Server가 객체 메모리 해제
        delete it->second;
        // map 항목 삭제
        m_channels.erase(it);
    }
}

// [changed 0807] m_nicknames를 정규화 키(IRC Lowercase)로 저장하므로
// O(log N)의 std::map::find로 직접 조회합니다.
Client* Server::getClientByNick(const std::string& nickname)
{
    std::map<std::string, Client*>::iterator it = m_nicknames.find(Utils::toIRCLower(nickname));
    if (it != m_nicknames.end())
        return it->second;
    return NULL;
}