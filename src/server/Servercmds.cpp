#include "server/Server.hpp"
#include "client/Client.hpp"
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

const std::map<std::string, Channel*>& Server::getChannels() const
{
    return m_channels;
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

// [Refactor Notice] 닉네임 동기화 불일치(댕글링 포인터/고스트 닉네임 버그) 방지를 위해
// m_nicknames 맵을 제거하고, m_clients를 Single Source of Truth로 사용하여
// O(N) 순회 방식으로 클라이언트를 조회하도록 변경되었습니다.
// (함수 시그니처 및 반환값 인터페이스는 기존과 100% 동일하게 유지됩니다.)
Client* Server::getClientByNick(const std::string& nickname)
{
    if (nickname.empty())
        return NULL;
    for (std::map<int, Client*>::iterator it = m_clients.begin(); it != m_clients.end(); ++it)
    {
        if (it->second && Utils::isSameNickname(it->second->getNickname(), nickname))
            return it->second;
    }
    return NULL;
}

// 유저가 참여한 채널 개수 세는 함수
// Server가 갖고 있는 m_channels을 순회하며 해당 클라이언트가 들어가 있는 채널 수를 카운팅
size_t Server::getUserJoinedChannelCount(Client* client) const
{
    size_t count = 0;
    for (std::map<std::string, Channel*>::const_iterator it = m_channels.begin(); it != m_channels.end(); ++it)
    {
        if (it->second && it->second->isMember(client))
            count++;
    }
    return count;
}
