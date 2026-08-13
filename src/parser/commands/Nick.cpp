#include "parser/commands/Nick.hpp"
#include "parser/Message.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "channel/Channel.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"

#include <iostream>
#include <set>

Nick::Nick() {}
Nick::Nick(const Nick& other) : ICommand(other) {}
Nick& Nick::operator=(const Nick& other)
{
    (void)other;
    return *this;
}
Nick::~Nick() {}

void Nick::execute(Server& server, Client& client, const Message& msg)
{
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    std::string nickname = "";
    if (!msg.getParams().empty())
        nickname = msg.getParams()[0];
    else if (msg.hasTrailing())
        nickname = msg.getTrailing();

    if (nickname.empty())
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NONICKNAMEGIVEN, target, ":No nickname given"));
        return;
    }

    if (Utils::isSameNickname(nickname, client.getNickname()))
        return; // 이미 쓰고 있는 닉네임과 동일 (RFC1459 대소문자/특수문자 무시) — 에러 아님, 무시

    // NICK 파라미터 누락, 유효하지 않은 문자열(ERR_ERRONEUSNICKNAME 432), 닉네임 중복(ERR_NICKNAMEINUSE 433)
    // 발생 시 에러 응답 후 세션을 유지(markForDeletion 미호출)하고 닉네임을 변경하지 않아 인증 대기 상태를 유지합니다.
    if (!Utils::isValidNickname(nickname))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_ERRONEUSNICKNAME, target, nickname + " :Erroneous nickname"));
        return;
    }
    if (server.isNicknameInUse(nickname))
    {
        client.appendToOutBuffer(reply(Numeric::ERR_NICKNAMEINUSE, target, nickname + " :Nickname is already in use"));
        return;
    }

    // 1. 변경 전 등록 상태 및 메시지 스냅샷 (기존 buildMessage 재활용)
    const bool wasRegistered = client.isRegistered();
    std::string nickMsg = "";
    if (wasRegistered && !client.getNickname().empty())
    {
        nickMsg = buildMessage(client, "NICK", "", nickname);
    }

    // 2. 닉네임 설정 (m_clients가 single source of truth이므로 client.setNickname만으로 서버 전체에 즉시 반영)
    client.setNickname(nickname);

    // 3. 최초 등록 완료 처리 (001 RPL_WELCOME)
    if (!client.isRegistered() && client.hasCorrectPassword() && !client.getUsername().empty())
    {
        client.setRegistered(true);
        client.appendToOutBuffer(reply(Numeric::RPL_WELCOME, nickname, ":Welcome to the IRC network, " + nickname));
    }

    // 4. 이미 등록된 클라이언트가 닉네임을 변경한 경우 브로드캐스트 (본인 + 공유 채널 멤버, 중복 제거)
    if (wasRegistered && !nickMsg.empty())
    {
        std::set<Client*> recipients;
        recipients.insert(&client);

        const std::map<std::string, Channel*>& channels = server.getChannels();
        for (std::map<std::string, Channel*>::const_iterator it = channels.begin(); it != channels.end(); ++it)
        {
            if (it->second && it->second->isMember(&client))
            {
                const std::map<Client*, bool>& members = it->second->getMembers();
                for (std::map<Client*, bool>::const_iterator mIt = members.begin(); mIt != members.end(); ++mIt)
                {
                    if (mIt->first)
                        recipients.insert(mIt->first);
                }
            }
        }

        for (std::set<Client*>::iterator it = recipients.begin(); it != recipients.end(); ++it)
        {
            (*it)->appendToOutBuffer(nickMsg);
        }
    }
}
