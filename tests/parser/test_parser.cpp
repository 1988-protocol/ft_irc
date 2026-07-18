#include "parser/Message.hpp"
#include "parser/Parser.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"

#include <iostream>
#include <string>
#include <vector>

// 42 규정상 외부 라이브러리(gtest 등)를 쓸 수 없어, main() + check() 헬퍼로 된
// 최소한의 assert 기반 하네스를 직접 만든다. 실패해도 나머지 케이스를 계속 돌려
// 한 번의 실행으로 전체 결과를 다 보게 한다(CLAUDE.md 6절 검증 게이트와 연동:
// 종료 코드 0 = 전부 통과 = 커밋 가능).
namespace
{
    int g_failureCount = 0;

    void check(bool condition, const std::string& description)
    {
        if (condition)
        {
            std::cout << "PASS: " << description << std::endl;
        }
        else
        {
            std::cout << "FAIL: " << description << std::endl;
            ++g_failureCount;
        }
    }

    void testMessageParsing()
    {
        // case 1: 기본 커맨드+파라미터
        {
            Message msg = Message::parse("NICK bob");
            check(msg.getPrefix().empty(), "case1: prefix empty");
            check(msg.getCommand() == "NICK", "case1: command == NICK");
            check(msg.getParams().size() == 1 && msg.getParams()[0] == "bob", "case1: params == [bob]");
            check(!msg.hasTrailing(), "case1: no trailing");
        }
        // case 2: prefix + trailing
        {
            Message msg = Message::parse(":irc.example.com NOTICE bob :Welcome");
            check(msg.getPrefix() == "irc.example.com", "case2: prefix == irc.example.com");
            check(msg.getCommand() == "NOTICE", "case2: command == NOTICE");
            check(msg.getParams().size() == 1 && msg.getParams()[0] == "bob", "case2: params == [bob]");
            check(msg.hasTrailing() && msg.getTrailing() == "Welcome", "case2: trailing == Welcome");
        }
        // case 3: 다중 파라미터 + trailing
        {
            Message msg = Message::parse("USER bob 0 * :Bob Real Name");
            check(msg.getCommand() == "USER", "case3: command == USER");
            check(msg.getParams().size() == 3
                && msg.getParams()[0] == "bob" && msg.getParams()[1] == "0" && msg.getParams()[2] == "*",
                "case3: params == [bob, 0, *]");
            check(msg.hasTrailing() && msg.getTrailing() == "Bob Real Name", "case3: trailing == Bob Real Name");
        }
        // case 4: 중복 공백(파라미터 사이) + trailing 내부 공백 보존
        {
            Message msg = Message::parse("PRIVMSG bob   :hello   world");
            check(msg.getParams().size() == 1 && msg.getParams()[0] == "bob", "case4: params == [bob] (중복 공백 접힘)");
            check(msg.hasTrailing() && msg.getTrailing() == "hello   world",
                "case4: trailing == 'hello   world' (내부 공백 보존)");
        }
        // case 5: 파라미터 없는 PASS (디스패처가 나중에 461을 유도할 입력)
        {
            Message msg = Message::parse("PASS   ");
            check(msg.getCommand() == "PASS", "case5: command == PASS");
            check(msg.getParams().empty(), "case5: params empty");
            check(!msg.hasTrailing(), "case5: no trailing");
        }
        // case 6: 빈 trailing(콜론만) — trailing 없음과 구분되어야 함
        {
            Message msg = Message::parse(":nick!user@host PRIVMSG #chan :");
            check(msg.getPrefix() == "nick!user@host", "case6: prefix == nick!user@host");
            check(msg.getParams().size() == 1 && msg.getParams()[0] == "#chan", "case6: params == [#chan]");
            check(msg.hasTrailing() && msg.getTrailing().empty(), "case6: hasTrailing==true, trailing==''");
        }
        // case 7: 빈 줄
        {
            Message msg = Message::parse("");
            check(msg.getCommand().empty(), "case7: empty line -> command empty (dispatcher ignores)");
        }
        // case 8: 공백만 있는 줄
        {
            Message msg = Message::parse("   ");
            check(msg.getCommand().empty(), "case8: whitespace-only line -> command empty");
        }
        // case 9: PING의 trailing 토큰
        {
            Message msg = Message::parse("PING :12345");
            check(msg.getCommand() == "PING", "case9: command == PING");
            check(msg.getParams().empty(), "case9: params empty");
            check(msg.hasTrailing() && msg.getTrailing() == "12345", "case9: trailing == 12345");
        }
        // case 10: prefix 내부에 콜론이 여러 개 있어도 불투명하게 처리
        {
            Message msg = Message::parse(":a:b:c NICK newnick");
            check(msg.getPrefix() == "a:b:c", "case10: prefix == a:b:c");
            check(msg.getCommand() == "NICK", "case10: command == NICK");
            check(msg.getParams().size() == 1 && msg.getParams()[0] == "newnick", "case10: params == [newnick]");
            check(!msg.hasTrailing(), "case10: no trailing");
        }
    }

    void testDispatcherAndRegistration()
    {
        Parser parser;
        Server server("testpass");

        // 미등록 상태에서 등록 계열 외 커맨드는 451
        {
            Client client;
            parser.process(server, client, "JOIN #x");
            check(client.getOutbox().find(" 451 ") != std::string::npos,
                "dispatch: unregistered + JOIN -> 451 ERR_NOTREGISTERED");
        }

        // 인자 없는 NICK -> 431
        {
            Client client;
            parser.process(server, client, "NICK");
            check(client.getOutbox().find(" 431 ") != std::string::npos,
                "dispatch: NICK without arg -> 431 ERR_NONICKNAMEGIVEN");
        }

        // 중복 닉네임 -> 433 (먼저 다른 클라이언트가 taken을 선점)
        {
            Client owner;
            server.registerNickname("taken", owner);

            Client challenger;
            parser.process(server, challenger, "NICK taken");
            check(challenger.getOutbox().find(" 433 ") != std::string::npos,
                "dispatch: duplicate NICK -> 433 ERR_NICKNAMEINUSE");
        }

        // PASS -> NICK -> USER 해피패스: 최종적으로 001 수신
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK alice");
            parser.process(server, client, "USER alice 0 * :Alice Real Name");
            check(client.isRegistered(), "dispatch: happy path -> client.isRegistered() == true");
            check(client.getOutbox().find(" 001 ") != std::string::npos,
                "dispatch: happy path -> outbox contains 001 RPL_WELCOME");
        }
    }

    // CLAUDE.md 7절: 이미 구현된 numeric reply 경로 중 위 해피/451/431/433 케이스에
    // 가려져 있던 나머지(421/432/461x2/462x2/464)를 리뷰에서 지적받아 추가한다.
    void testAdditionalErrorPaths()
    {
        Parser parser;
        Server server("testpass");

        // 완전히 모르는 커맨드 -> 421
        // (미등록 상태면 isAllowedBeforeRegistration()에서 걸려 451이 먼저 나가므로,
        // map 조회 자체가 실패하는 421 경로를 보려면 등록을 먼저 마쳐야 한다.)
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK ivan");
            parser.process(server, client, "USER ivan 0 * :Ivan Real Name");
            client.clearOutbox();
            parser.process(server, client, "FOOBAR arg");
            check(client.getOutbox().find(" 421 ") != std::string::npos,
                "dispatch: unknown command -> 421 ERR_UNKNOWNCOMMAND");
        }

        // 형식 위반 닉네임(숫자로 시작) -> 432
        {
            Client client;
            parser.process(server, client, "NICK 1bad");
            check(client.getOutbox().find(" 432 ") != std::string::npos,
                "dispatch: malformed NICK -> 432 ERR_ERRONEUSNICKNAME");
        }

        // 인자 없는 PASS -> 461
        {
            Client client;
            parser.process(server, client, "PASS");
            check(client.getOutbox().find(" 461 ") != std::string::npos,
                "dispatch: PASS without arg -> 461 ERR_NEEDMOREPARAMS");
        }

        // 파라미터 부족한 USER(trailing 없음) -> 461
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK bob");
            parser.process(server, client, "USER bob 0 *");
            check(client.getOutbox().find(" 461 ") != std::string::npos,
                "dispatch: USER without trailing -> 461 ERR_NEEDMOREPARAMS");
        }

        // 비밀번호 불일치 -> 464
        {
            Client client;
            parser.process(server, client, "PASS wrongpass");
            check(client.getOutbox().find(" 464 ") != std::string::npos,
                "dispatch: wrong PASS -> 464 ERR_PASSWDMISMATCH");
        }

        // 이미 등록된 클라이언트의 재-PASS -> 462
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK carol");
            parser.process(server, client, "USER carol 0 * :Carol Real Name");
            check(client.isRegistered(), "setup: carol registered before re-PASS check");
            parser.process(server, client, "PASS testpass");
            check(client.getOutbox().find(" 462 ") != std::string::npos,
                "dispatch: re-PASS after registration -> 462 ERR_ALREADYREGISTRED");
        }

        // 이미 등록된 클라이언트의 재-USER -> 462
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK dave");
            parser.process(server, client, "USER dave 0 * :Dave Real Name");
            check(client.isRegistered(), "setup: dave registered before re-USER check");
            parser.process(server, client, "USER dave 0 * :Dave Again");
            check(client.getOutbox().find(" 462 ") != std::string::npos,
                "dispatch: re-USER after registration -> 462 ERR_ALREADYREGISTRED");
        }
    }

    // 리뷰에서 지적된 두 버그(자기 자신과 동일 닉네임 재전송 시 오탐 433, 닉네임 변경/QUIT
    // 시 이전 닉네임 미반환)에 대한 회귀 테스트. irc/md/parser_message_grammar.md 4.6 참고.
    void testNicknameLifecycleRegressions()
    {
        Parser parser;
        Server server("testpass");

        // 자기 자신과 동일한 닉네임 재전송은 433이 아니라 no-op이어야 한다
        {
            Client client;
            parser.process(server, client, "NICK erin");
            client.clearOutbox();
            parser.process(server, client, "NICK erin");
            check(client.getOutbox().find(" 433 ") == std::string::npos,
                "regression: re-sending own current nickname must NOT produce 433");
            check(client.getNickname() == "erin",
                "regression: nickname unchanged after self-resend");
        }

        // 닉네임 변경 후 이전 닉네임을 다른 클라이언트가 다시 쓸 수 있어야 한다
        {
            Client first;
            parser.process(server, first, "NICK frank");
            parser.process(server, first, "NICK george"); // frank -> george

            Client second;
            parser.process(server, second, "NICK frank"); // 이전 닉네임 재사용
            check(second.getOutbox().find(" 433 ") == std::string::npos,
                "regression: nickname released on change is reusable by another client");
            check(second.getNickname() == "frank",
                "regression: second client successfully acquired released nickname");
        }

        // QUIT 처리 후 닉네임을 다른 클라이언트가 다시 쓸 수 있어야 한다
        {
            Client first;
            parser.process(server, first, "NICK harry");
            parser.process(server, first, "QUIT :bye");

            Client second;
            parser.process(server, second, "NICK harry");
            check(second.getOutbox().find(" 433 ") == std::string::npos,
                "regression: nickname released on QUIT is reusable by another client");
            check(second.getNickname() == "harry",
                "regression: second client successfully acquired nickname freed by QUIT");
        }
    }
}

int main()
{
    testMessageParsing();
    testDispatcherAndRegistration();
    testAdditionalErrorPaths();
    testNicknameLifecycleRegressions();

    std::cout << "----" << std::endl;
    if (g_failureCount == 0)
    {
        std::cout << "ALL TESTS PASSED" << std::endl;
        return 0;
    }
    std::cout << g_failureCount << " TEST(S) FAILED" << std::endl;
    return 1;
}
