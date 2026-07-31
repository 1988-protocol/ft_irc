#include "parser/Parser.hpp"
#include "parser/Message.hpp"
#include "common/ICommand.hpp"
#include "common/Replies.hpp"
#include "common/Utils.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"

#include "parser/commands/Pass.hpp"
#include "parser/commands/Nick.hpp"
#include "parser/commands/User.hpp"
#include "parser/commands/Ping.hpp"
#include "parser/commands/Pong.hpp"
#include "parser/commands/Quit.hpp"
// #include "parser/commands/Join.hpp"
// #include "parser/commands/Part.hpp"
// #include "parser/commands/Kick.hpp"
// #include "parser/commands/Invite.hpp"
// #include "parser/commands/Topic.hpp"
// #include "parser/commands/Mode.hpp"
// #include "parser/commands/Privmsg.hpp"
// #include "parser/commands/Notice.hpp"

Parser::Parser()
{
    registerCommand("PASS", new Pass());
    registerCommand("NICK", new Nick());
    registerCommand("USER", new User());
    registerCommand("PING", new Ping());
    registerCommand("PONG", new Pong());
    registerCommand("QUIT", new Quit());
    // registerCommand("JOIN", new Join());
    // registerCommand("PART", new Part());
    // registerCommand("KICK", new Kick());
    // registerCommand("INVITE", new Invite());
    // registerCommand("TOPIC", new Topic());
    // registerCommand("MODE", new Mode());
    // registerCommand("PRIVMSG", new Privmsg());
    // registerCommand("NOTICE", new Notice());
}

Parser::~Parser()
{
    for (std::map<std::string, ICommand*>::iterator it = m_commands.begin(); it != m_commands.end(); ++it)
        delete it->second;
}

void Parser::registerCommand(const std::string& name, ICommand* handler)
{
    m_commands[name] = handler;
}

bool Parser::isAllowedBeforeRegistration(const std::string& command) const // 로그인 전에 사용가능한 명령어
{
    return command == "PASS" || command == "NICK" || command == "USER"
        || command == "QUIT" || command == "PING" || command == "PONG";
}

void Parser::process(Server& server, Client& client, const std::string& rawLine)
{
    Message msg = Message::parse(rawLine);
    if (msg.getCommand().empty())
        return; // 빈 줄/공백줄은 조용히 무시한다(RFC상 관용적으로 허용되는 관행)

    std::string command = Utils::toUpper(msg.getCommand());
    std::string target = client.getNickname().empty() ? "*" : client.getNickname();

    if (!client.isRegistered() && !isAllowedBeforeRegistration(command)) // 로그인을 안 했거나 로그인 전에 쓸 수 있는 명령어가 아니라면
    {
        client.queueReply(reply(Numeric::ERR_NOTREGISTERED, target, ":You have not registered"));
        return;
    }

    std::map<std::string, ICommand*>::iterator it = m_commands.find(command);
    if (it == m_commands.end())
    {
        client.queueReply(reply(Numeric::ERR_UNKNOWNCOMMAND, target, command + " :Unknown command"));
        return;
    }

    it->second->execute(server, client, msg);
}
