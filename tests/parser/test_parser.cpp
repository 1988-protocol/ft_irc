#include "parser/Message.hpp"
#include "parser/Parser.hpp"
#include "client/Client.hpp"
#include "server/Server.hpp"
#include "common/Utils.hpp"

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
        // case 11: 탭 문자 구분 방지 테스트 (Note 1)
        {
            Message msg = Message::parse("PRIVMSG #chan\twith_tab :hello");
            check(msg.getParams().size() == 1 && msg.getParams()[0] == "#chan\twith_tab",
                  "case11: Tab character should be treated as part of the token, not a delimiter");
        }
        // case 12: 15번째 파라미터 자동 Trailing 테스트 (콜론 없는 경우)
        {
            Message msg = Message::parse("CMD p1 p2 p3 p4 p5 p6 p7 p8 p9 p10 p11 p12 p13 p14 p15 trailing content");
            check(msg.getParams().size() == 14, "case12: Should contain exactly 14 middle parameters");
            check(msg.hasTrailing() && msg.getTrailing() == "p15 trailing content",
                  "case12: The 15th parameter must automatically become trailing and preserve spaces");
        }
        // case 13: 15번째 파라미터 자동 Trailing 테스트 (콜론 있는 경우)
        {
            Message msg = Message::parse("CMD p1 p2 p3 p4 p5 p6 p7 p8 p9 p10 p11 p12 p13 p14 :p15 trailing content");
            check(msg.getParams().size() == 14, "case13: Should contain exactly 14 middle parameters");
            check(msg.hasTrailing() && msg.getTrailing() == "p15 trailing content",
                  "case13: The 15th parameter with colon prefix is parsed correctly");
        }
        // case 14: 콜론이 공백 없이 바로 붙어 있는 경우 (Trailing 마커 아님 검증)
        {
            Message msg = Message::parse("PRIVMSG #channel:name :hello");
            check(msg.getParams().size() == 1 && msg.getParams()[0] == "#channel:name",
                  "case14: Colon without preceding space must be treated as middle parameter part");
        }

        // 만약 네트워크 단에서 처리하게 되면 case 15를 삭제하면 된다.
        // case 15: NUL (\0) 문자 포함된 메시지 유입 차단 테스트
        {
            std::string rawInput = "NICK bo";
            rawInput.push_back('\0');
            rawInput += "b";
            Message msg = Message::parse(rawInput);
            check(msg.getCommand().empty(), "case15: Message containing NUL must be rejected (command empty)");
        }
    }

    void testDispatcherAndRegistration()
    {
        Parser parser;
        // [Change] real Server 생성자 (int port, const std::string& password) 호출
        Server server(6667, "testpass");

        // 미등록 상태에서 등록 계열 외 커맨드는 451
        {
            Client client;
            parser.process(server, client, "JOIN #x");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 451 ") != std::string::npos,
                "dispatch: unregistered + JOIN -> 451 ERR_NOTREGISTERED");
        }

        // 인자 없는 NICK -> 431
        {
            Client client;
            parser.process(server, client, "NICK");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 431 ") != std::string::npos,
                "dispatch: NICK without arg -> 431 ERR_NONICKNAMEGIVEN");
        }

        // 중복 닉네임 -> 433 (먼저 다른 클라이언트가 taken을 선점)
        {
            Client owner;
            parser.process(server, owner, "NICK taken");

            Client challenger;
            parser.process(server, challenger, "NICK taken");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(challenger.getOutBuffer().find(" 433 ") != std::string::npos,
                "dispatch: duplicate NICK -> 433 ERR_NICKNAMEINUSE");
        }

        // PASS -> NICK -> USER 해피패스: 최종적으로 001 수신
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK alice");
            parser.process(server, client, "USER alice 0 * :Alice Real Name");
            check(client.isRegistered(), "dispatch: happy path -> client.isRegistered() == true");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 001 ") != std::string::npos,
                "dispatch: happy path -> outbox contains 001 RPL_WELCOME");
        }
    }

    // CLAUDE.md 7절: 이미 구현된 numeric reply 경로 중 위 해피/451/431/433 케이스에
    // 가려져 있던 나머지(421/432/461x2/462x2/464)를 리뷰에서 지적받아 추가한다.
    void testAdditionalErrorPaths()
    {
        Parser parser;
        // [Change] real Server 생성자 (int port, const std::string& password) 호출
        Server server(6667, "testpass");

        // 완전히 모르는 커맨드 -> 421
        // (미등록 상태면 isAllowedBeforeRegistration()에서 걸려 451이 먼저 나가므로,
        // map 조회 자체가 실패하는 421 경로를 보려면 등록을 먼저 마쳐야 한다.)
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK ivan");
            parser.process(server, client, "USER ivan 0 * :Ivan Real Name");
            // [Change] clearOutbox() 대신 getOutBuffer().clear() 사용
            client.getOutBuffer().clear();
            parser.process(server, client, "FOOBAR arg");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 421 ") != std::string::npos,
                "dispatch: unknown command -> 421 ERR_UNKNOWNCOMMAND");
        }

        // 형식 위반 닉네임(숫자로 시작) -> 432
        {
            Client client;
            parser.process(server, client, "NICK 1bad");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 432 ") != std::string::npos,
                "dispatch: malformed NICK -> 432 ERR_ERRONEUSNICKNAME");
        }

        // 형식 위반 닉네임(특수문자로 시작) -> 432 (RFC 1459는 첫 글자로 알파벳만 허용)
        {
            Client client;
            parser.process(server, client, "NICK [bad");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 432 ") != std::string::npos,
                "dispatch: NICK starting with special -> 432 ERR_ERRONEUSNICKNAME");
        }

        // 형식 위반 닉네임(RFC 1459 비허용 특수문자 _ 포함) -> 432
        {
            Client client;
            parser.process(server, client, "NICK bad_nick");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 432 ") != std::string::npos,
                "dispatch: NICK with underscore -> 432 ERR_ERRONEUSNICKNAME");
        }

        // 형식 위반 닉네임(RFC 1459 비허용 특수문자 | 포함) -> 432
        {
            Client client;
            parser.process(server, client, "NICK bad|nick");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 432 ") != std::string::npos,
                "dispatch: NICK with pipe -> 432 ERR_ERRONEUSNICKNAME");
        }

        // 형식 위반 닉네임(9자 초과) -> 432
        {
            Client client;
            parser.process(server, client, "NICK toolongnickname");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 432 ") != std::string::npos,
                "dispatch: NICK longer than 9 chars -> 432 ERR_ERRONEUSNICKNAME");
        }

        // 인자 없는 PASS -> 461
        {
            Client client;
            parser.process(server, client, "PASS");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 461 ") != std::string::npos,
                "dispatch: PASS without arg -> 461 ERR_NEEDMOREPARAMS");
        }

        // 파라미터 부족한 USER(trailing 없음) -> 461
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK bob");
            parser.process(server, client, "USER bob 0 *");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 461 ") != std::string::npos,
                "dispatch: USER without trailing -> 461 ERR_NEEDMOREPARAMS");
        }

        // 비밀번호 불일치 -> 464
        {
            Client client;
            parser.process(server, client, "PASS wrongpass");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 464 ") != std::string::npos,
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
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 462 ") != std::string::npos,
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
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 462 ") != std::string::npos,
                "dispatch: re-USER after registration -> 462 ERR_ALREADYREGISTRED");
        }
    }

    // 리뷰에서 지적된 두 버그(자기 자신과 동일 닉네임 재전송 시 오탐 433, 닉네임 변경/QUIT
    // 시 이전 닉네임 미반환)에 대한 회귀 테스트. irc/md/parser_message_grammar.md 4.6 참고.
    void testNicknameLifecycleRegressions()
    {
        Parser parser;
        // [Change] real Server 생성자 (int port, const std::string& password) 호출
        Server server(6667, "testpass");

        // 자기 자신과 동일한 닉네임 재전송은 433이 아니라 no-op이어야 한다
        {
            Client client;
            parser.process(server, client, "NICK erin");
            // [Change] clearOutbox() 대신 getOutBuffer().clear() 사용
            client.getOutBuffer().clear();
            parser.process(server, client, "NICK erin");
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(client.getOutBuffer().find(" 433 ") == std::string::npos,
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
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(second.getOutBuffer().find(" 433 ") == std::string::npos,
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
            // [Change] getOutbox() 대신 real Client의 getOutBuffer() 사용
            check(second.getOutBuffer().find(" 433 ") == std::string::npos,
                "regression: nickname released on QUIT is reusable by another client");
            check(second.getNickname() == "harry",
                "regression: second client successfully acquired nickname freed by QUIT");
        }
    }

    void testPingCommandValidation()
    {
        Parser parser;
        Server server(6667, "testpass");

        // 1. PING 파라미터 없음 -> 409 ERR_NOORIGIN
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK pinguser1");
            parser.process(server, client, "USER pinguser1 0 * :Ping User");
            check(client.isRegistered() == true, "setup: pinguser1 registered successfully");
            client.getOutBuffer().clear();

            parser.process(server, client, "PING");
            check(client.getOutBuffer().find(" 409 ") != std::string::npos,
                "ping: PING without parameters returns 409 ERR_NOORIGIN");
        }

        // 2. PING token (단일 파라미터) -> PONG :token
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK pinguser2");
            parser.process(server, client, "USER pinguser2 0 * :Ping User");
            check(client.isRegistered() == true, "setup: pinguser2 registered successfully");
            client.getOutBuffer().clear();

            parser.process(server, client, "PING token123");
            check(client.getOutBuffer().find(" PONG ") != std::string::npos && client.getOutBuffer().find(":token123") != std::string::npos,
                "ping: PING single middle param returns PONG token123");
        }

        // 3. PING :token (trailing 단일 파라미터) -> PONG :token
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK pinguser3");
            parser.process(server, client, "USER pinguser3 0 * :Ping User");
            check(client.isRegistered() == true, "setup: pinguser3 registered successfully");
            client.getOutBuffer().clear();

            parser.process(server, client, "PING :token456");
            check(client.getOutBuffer().find(" PONG ") != std::string::npos && client.getOutBuffer().find(":token456") != std::string::npos,
                "ping: PING single trailing param returns PONG token456");
        }

        // 4. PING server1 :server2 (2개 파라미터: server1=token, server2=target server)
        // server2가 존재하지 않는 서버일 경우 -> 402 ERR_NOSUCHSERVER
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK pinguser4");
            parser.process(server, client, "USER pinguser4 0 * :Ping User");
            check(client.isRegistered() == true, "setup: pinguser4 registered successfully");
            client.getOutBuffer().clear();

            parser.process(server, client, "PING server1 :unknown_server");
            check(client.getOutBuffer().find(" 402 ") != std::string::npos,
                "ping: PING with invalid server2 returns 402 ERR_NOSUCHSERVER");
        }

        // 5. PING server1 :server2 (server2가 본인 서버 이름일 경우 위치 보존하여 server1을 token으로 응답)
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK pinguser5");
            parser.process(server, client, "USER pinguser5 0 * :Ping User");
            check(client.isRegistered() == true, "setup: pinguser5 registered successfully");
            client.getOutBuffer().clear();

            std::string serverName = getServerName();
            parser.process(server, client, "PING server1 :" + serverName);
            check(client.getOutBuffer().find(" PONG ") != std::string::npos && client.getOutBuffer().find(":server1") != std::string::npos,
                "ping: PING server1 :server2 preserves position and replies PONG with server1");
        }

        // 6. PING server1 :SERVER2 (server2 대소문자 무시 검증)
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK pinguser6");
            parser.process(server, client, "USER pinguser6 0 * :Ping User");
            check(client.isRegistered() == true, "setup: pinguser6 registered successfully");
            client.getOutBuffer().clear();

            std::string upperServerName = Utils::toUpper(getServerName());
            parser.process(server, client, "PING server1 :" + upperServerName);
            check(client.getOutBuffer().find(" PONG ") != std::string::npos && client.getOutBuffer().find(":server1") != std::string::npos,
                "ping: PING with case-insensitive server2 replies PONG correctly");
        }
    }

    void testRegistrationErrorRecovery()
    {
        Parser parser;
        Server server(6667, "testpass");

        // 시나리오 1: PASS 틀림 -> 세션 유지 및 미등록 상태 -> 올바른 PASS 전송 후 NICK/USER 수신 시 정상 등록
        {
            Client client;
            parser.process(server, client, "PASS wrongpass");
            check(client.getOutBuffer().find(" 464 ") != std::string::npos, "recovery: wrong PASS returns 464");
            check(client.needsDisconnect() == false, "recovery: wrong PASS keeps session alive");
            check(client.isRegistered() == false, "recovery: wrong PASS leaves client unregistered");

            client.getOutBuffer().clear();
            parser.process(server, client, "PASS testpass");
            check(client.hasCorrectPassword() == true, "recovery: resending correct PASS succeeds");

            parser.process(server, client, "NICK passuser");
            parser.process(server, client, "USER passuser 0 * :Pass User");
            check(client.isRegistered() == true, "recovery: registration completes after correcting PASS");
            check(client.getOutBuffer().find(" 001 ") != std::string::npos, "recovery: receives 001 RPL_WELCOME after correcting PASS");
        }

        // 시나리오 2: NICK 형식 오류 -> 세션 유지 및 닉네임 미설정 -> 올바른 NICK 전송 후 정상 등록
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK 1badnick");
            check(client.getOutBuffer().find(" 432 ") != std::string::npos, "recovery: malformed NICK returns 432");
            check(client.needsDisconnect() == false, "recovery: malformed NICK keeps session alive");
            check(client.getNickname().empty(), "recovery: malformed NICK leaves nickname empty");
            check(client.isRegistered() == false, "recovery: malformed NICK leaves client unregistered");

            client.getOutBuffer().clear();
            parser.process(server, client, "USER nickuser 0 * :Nick User");
            check(client.isRegistered() == false, "recovery: USER before valid NICK leaves client unregistered");

            parser.process(server, client, "NICK validnick");
            check(client.getNickname() == "validnick", "recovery: resending valid NICK sets nickname");
            check(client.isRegistered() == true, "recovery: registration completes upon receiving valid NICK");
            check(client.getOutBuffer().find(" 001 ") != std::string::npos, "recovery: receives 001 RPL_WELCOME upon valid NICK");
        }

        // 시나리오 3: USER 파라미터 부족 -> 세션 유지 -> 올바른 USER 전송 후 정상 등록
        {
            Client client;
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK usernick");
            parser.process(server, client, "USER incomplete");
            check(client.getOutBuffer().find(" 461 ") != std::string::npos, "recovery: incomplete USER returns 461");
            check(client.needsDisconnect() == false, "recovery: incomplete USER keeps session alive");
            check(client.getUsername().empty(), "recovery: incomplete USER leaves username empty");
            check(client.isRegistered() == false, "recovery: incomplete USER leaves client unregistered");

            client.getOutBuffer().clear();
            parser.process(server, client, "USER usernick 0 * :User Real Name");
            check(client.getUsername() == "usernick", "recovery: resending valid USER sets username");
            check(client.isRegistered() == true, "recovery: registration completes upon receiving valid USER");
            check(client.getOutBuffer().find(" 001 ") != std::string::npos, "recovery: receives 001 RPL_WELCOME upon valid USER");
        }

        // 시나리오 4: 연쇄 오류 발생 후 교정을 통한 최종 정상 등록
        {
            Client client;
            parser.process(server, client, "PASS wrong1"); // 464
            parser.process(server, client, "NICK @badnick"); // 432
            parser.process(server, client, "USER baduser"); // 461
            check(client.needsDisconnect() == false, "recovery: multi-error keeps session alive");
            check(client.isRegistered() == false, "recovery: multi-error leaves client unregistered");

            client.getOutBuffer().clear();
            parser.process(server, client, "PASS testpass");
            parser.process(server, client, "NICK combouser");
            parser.process(server, client, "USER combouser 0 * :Combo User");
            check(client.isRegistered() == true, "recovery: multi-error client fully registers after sending valid commands");
            check(client.getOutBuffer().find(" 001 ") != std::string::npos, "recovery: receives 001 RPL_WELCOME after full recovery");
        }
    }
}

int main()
{
    testMessageParsing();
    testDispatcherAndRegistration();
    testAdditionalErrorPaths();
    testNicknameLifecycleRegressions();
    testPingCommandValidation();
    testRegistrationErrorRecovery();

    std::cout << "----" << std::endl;
    if (g_failureCount == 0)
    {
        std::cout << "ALL TESTS PASSED" << std::endl;
        return 0;
    }
    std::cout << g_failureCount << " TEST(S) FAILED" << std::endl;
    return 1;
}
