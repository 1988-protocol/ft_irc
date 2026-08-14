import os
import sys
import socket
import subprocess
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_utils import read_until_eof, drain_process_output

def main():
    print("=== Starting IRC Integration Test ===")
    
    # 1. Start Server on port 10001
    port = 10001
    password = "testpassword"
    server_process = subprocess.Popen(["./ircserv", str(port), password], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    drain_process_output(server_process)  # stdout 파이프가 안 읽혀서 서버가 멈추는 것을 방지
    time.sleep(0.5) # Wait for server to bind and start listening

    s_alice = None
    s_bob = None
    s_dave = None
    s_eve = None
    s_oversize = None
    s_flooder = None
    s_sink = None
    try:
        # 2. Connect Alice (Client 1)
        s_alice = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_alice.connect(("localhost", port))
        s_alice.setblocking(False)
        
        # Send registration
        s_alice.sendall(f"PASS {password}\r\nNICK alice\r\nUSER alice 0 * :Alice Real\r\n".encode())
        time.sleep(0.1)
        resp = read_until_eof(s_alice)
        print("[Alice Registration Response]:")
        print(resp.strip())
        assert "001 alice" in resp, "Alice registration failed"

        # 3. Connect Bob (Client 2)
        s_bob = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_bob.connect(("localhost", port))
        s_bob.setblocking(False)
        
        # Test Duplicate Nickname Collision (Bob tries to take Alice's nickname)
        print("\n--- Bob attempts duplicate NICK (should get 433 ERR_NICKNAMEINUSE) ---")
        s_bob.sendall(f"PASS {password}\r\nNICK alice\r\n".encode())
        time.sleep(0.1)
        resp_dup = read_until_eof(s_bob)
        print("[Bob Duplicate NICK Response]:")
        print(resp_dup.strip())
        assert "433" in resp_dup, "Expected 433 ERR_NICKNAMEINUSE for duplicate nickname"

        # Now Bob registers with unique nickname 'bob'
        s_bob.sendall("NICK bob\r\nUSER bob 0 * :Bob Real\r\n".encode())
        time.sleep(0.1)
        resp = read_until_eof(s_bob)
        print("[Bob Registration Response]:")
        print(resp.strip())
        assert "001 bob" in resp, "Bob registration failed"

        # 4. Alice joins #testchannel (she should become OP since she created it)
        print("\n--- Alice joins #testchannel ---")
        s_alice.sendall(b"JOIN #testchannel\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        print("[Alice JOIN Response]:")
        print(resp_alice.strip())
        assert "JOIN #testchannel" in resp_alice, "JOIN broadcast not received"
        assert "@alice" in resp_alice, "Alice should be OP in NAMES list"

        # 5. Bob joins #testchannel
        print("\n--- Bob joins #testchannel ---")
        s_bob.sendall(b"JOIN #testchannel\r\n")
        time.sleep(0.1)
        resp_bob = read_until_eof(s_bob)
        print("[Bob JOIN Response]:")
        print(resp_bob.strip())
        assert "JOIN #testchannel" in resp_bob, "Bob JOIN failed"
        
        # Alice should also see Bob join (broadcast check)
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        print("[Alice sees Bob join]:")
        print(resp_alice.strip())
        assert "bob!bob@127.0.0.1 JOIN #testchannel" in resp_alice or "bob!bob@localhost JOIN #testchannel" in resp_alice or "JOIN #testchannel" in resp_alice, "Alice did not see Bob join"

        # 6. Alice sets TOPIC
        print("\n--- Alice sets topic ---")
        s_alice.sendall(b"TOPIC #testchannel :New Project Topic\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        resp_bob = read_until_eof(s_bob)
        print("[Alice TOPIC Response]:")
        print(resp_alice.strip())
        print("[Bob sees TOPIC update]:")
        print(resp_bob.strip())
        assert "TOPIC #testchannel :New Project Topic" in resp_bob, "Bob did not see topic update"

        # 7. Alice enables +t (topic op only), then Bob tries to change TOPIC (should fail)
        print("\n--- Bob tries to change topic (should fail) ---")
        s_alice.sendall(b"MODE #testchannel +t\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        s_bob.sendall(b"TOPIC #testchannel :Bob's Topic\r\n")
        time.sleep(0.1)
        resp_bob = read_until_eof(s_bob)
        print("[Bob response to unauthorized TOPIC change]:")
        print(resp_bob.strip())
        assert "482 bob" in resp_bob, "Expected 482 ERR_CHANOPRIVSNEEDED"

        # 8. Channel PRIVMSG broadcast
        print("\n--- Alice sends channel PRIVMSG ---")
        s_alice.sendall(b"PRIVMSG #testchannel :Hello team!\r\n")
        time.sleep(0.1)
        resp_bob = read_until_eof(s_bob)
        print("[Bob receives channel message]:")
        print(resp_bob.strip())
        assert "PRIVMSG #testchannel :Hello team!" in resp_bob, "Bob did not receive broadcast"

        # 9. 1:1 PRIVMSG
        print("\n--- Bob sends direct PRIVMSG to Alice ---")
        s_bob.sendall(b"PRIVMSG alice :Hi Alice, PM check.\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        print("[Alice receives direct message]:")
        print(resp_alice.strip())
        assert "PRIVMSG alice :Hi Alice, PM check." in resp_alice, "Alice did not receive PM"

        # 10. KICK Banned target (Alice kicks Bob)
        print("\n--- Alice kicks Bob ---")
        s_alice.sendall(b"KICK #testchannel bob :You are kicked\r\n")
        time.sleep(0.1)
        resp_bob = read_until_eof(s_bob)
        resp_alice = read_until_eof(s_alice)
        print("[Bob sees himself kicked]:")
        print(resp_bob.strip())
        print("[Alice sees kick confirmation]:")
        print(resp_alice.strip())
        assert "KICK #testchannel bob :You are kicked" in resp_bob, "Bob kick broadcast failed"

        # 11. Mode +i (Invite only)
        print("\n--- Alice sets channel mode to Invite Only (+i) ---")
        s_alice.sendall(b"MODE #testchannel +i\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        print("[Alice sets MODE +i response]:")
        print(resp_alice.strip())

        # 12. Bob tries to JOIN again (should fail because channel is Invite Only)
        print("\n--- Bob tries to join invite-only channel (should fail) ---")
        s_bob.sendall(b"JOIN #testchannel\r\n")
        time.sleep(0.1)
        resp_bob = read_until_eof(s_bob)
        print("[Bob JOIN response]:")
        print(resp_bob.strip())
        assert "473 bob" in resp_bob, "Expected 473 ERR_INVITEONLYCHAN"

        # 13. Alice invites Bob
        print("\n--- Alice invites Bob ---")
        s_alice.sendall(b"INVITE bob #testchannel\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        resp_bob = read_until_eof(s_bob)
        print("[Alice INVITE response]:")
        print(resp_alice.strip())
        print("[Bob receives INVITE notification]:")
        print(resp_bob.strip())
        assert "INVITE bob" in resp_bob and "#testchannel" in resp_bob, "Bob did not receive INVITE"

        # 14. Bob joins again (should succeed now after invitation)
        print("\n--- Bob joins channel after invitation ---")
        s_bob.sendall(b"JOIN #testchannel\r\n")
        time.sleep(0.1)
        resp_bob = read_until_eof(s_bob)
        print("[Bob JOIN response after invitation]:")
        print(resp_bob.strip())
        assert "JOIN #testchannel" in resp_bob, "Bob JOIN failed"

        # 15. Alice gives OP to Bob (+o)
        print("\n--- Alice promotes Bob to operator (+o) ---")
        s_alice.sendall(b"MODE #testchannel +o bob\r\n")
        time.sleep(0.1)
        resp_bob = read_until_eof(s_bob)
        print("[Bob sees MODE +o]:")
        print(resp_bob.strip())
        assert "MODE #testchannel +o bob" in resp_bob, "MODE +o broadcast failed"

        # 16. Bob sets client limit to 2 (+l 2)
        print("\n--- Bob sets channel client limit (+l 2) ---")
        s_bob.sendall(b"MODE #testchannel +l 2\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        resp_bob_confirm = read_until_eof(s_bob)
        print("[Alice sees MODE +l 2]:")
        print(resp_alice.strip())

        # 16.5 Multi-channel NICK broadcast & duplicate prevention test
        # Alice and Bob join a second shared channel (#secondchan)
        print("\n--- Alice and Bob join #secondchan ---")
        s_alice.sendall(b"JOIN #secondchan\r\n")
        s_bob.sendall(b"JOIN #secondchan\r\n")
        time.sleep(0.1)
        read_until_eof(s_alice)
        read_until_eof(s_bob)

        # Alice changes nickname to 'wonder' (<= 9 chars per RFC 1459)
        print("\n--- Alice changes NICK to wonder across multiple shared channels ---")
        s_alice.sendall(b"NICK :wonder\r\n")
        time.sleep(0.1)
        resp_alice_nick = read_until_eof(s_alice)
        resp_bob_nick = read_until_eof(s_bob)
        print("[Alice NICK response]:")
        print(resp_alice_nick.strip())
        print("[Bob sees Alice NICK change]:")
        print(resp_bob_nick.strip())

        assert "NICK :wonder" in resp_alice_nick, "Alice did not receive NICK confirmation"
        assert ":alice!" in resp_bob_nick and " NICK :wonder\r\n" in resp_bob_nick, \
            "Bob did not receive Alice's valid NICK broadcast"
        # Duplicate prevention check: Bob must receive the NICK broadcast exactly ONCE despite 2 shared channels
        assert resp_bob_nick.count("NICK :wonder") == 1, \
            f"Expected exactly 1 NICK broadcast to Bob, but got {resp_bob_nick.count('NICK :wonder')}"

        # Bob sends message to 'wonder'
        print("\n--- Bob sends message to wonder ---")
        s_bob.sendall(b"PRIVMSG wonder :Hi Wonder!\r\n")
        time.sleep(0.1)
        resp_alice_pm = read_until_eof(s_alice)
        print("[Alice (wonder) receives PM]:")
        print(resp_alice_pm.strip())
        assert "PRIVMSG wonder :Hi Wonder!" in resp_alice_pm, "Alice did not receive PM to new nickname"

        # Change nickname back to 'alice' for ghost nickname test
        s_alice.sendall(b"NICK alice\r\n")
        time.sleep(0.1)
        read_until_eof(s_alice)
        read_until_eof(s_bob)

        # 16.6 Multi-channel QUIT broadcast & duplicate prevention test
        print("\n--- Alice sends graceful QUIT across multiple shared channels ---")
        s_alice.sendall(b"QUIT :Goodbye all!\r\n")
        time.sleep(0.1)
        resp_alice_quit = read_until_eof(s_alice)
        resp_bob_quit = read_until_eof(s_bob)
        print("[Alice QUIT response (ERROR closing link)]:")
        print(resp_alice_quit.strip())
        print("[Bob sees Alice QUIT broadcast]:")
        print(resp_bob_quit.strip())

        assert "ERROR :Closing Link: Goodbye all!" in resp_alice_quit, "Alice did not receive ERROR closing link"
        assert ":alice!" in resp_bob_quit and " QUIT :Goodbye all!\r\n" in resp_bob_quit, \
            "Bob did not receive Alice's valid QUIT broadcast"
        # Duplicate prevention check: Bob must receive the QUIT broadcast exactly ONCE despite 2 shared channels
        assert resp_bob_quit.count("QUIT :Goodbye all!") == 1, \
            f"Expected exactly 1 QUIT broadcast to Bob, but got {resp_bob_quit.count('QUIT :Goodbye all!')}"

        s_alice.close()
        s_alice = None
        time.sleep(0.2)

        # 17. Bob acquires released nickname 'alice' after graceful QUIT
        print("\n--- Bob acquires released nickname 'alice' after graceful QUIT ---")
        s_bob.sendall(b"NICK alice\r\n")
        time.sleep(0.1)
        resp_nick_take = read_until_eof(s_bob)
        print("[Bob NICK alice Response]:")
        print(resp_nick_take.strip())
        assert "433" not in resp_nick_take, "Server still thinks 'alice' is in use after graceful QUIT"

        # 18. Positive assertion: verify NICK change took effect
        print("\n--- Positive assertion: verify NICK change took effect ---")
        s_bob.sendall(b"PRIVMSG nobody :ping\r\n")
        time.sleep(0.1)
        resp_confirm = read_until_eof(s_bob)
        print("[Bob (now alice) PRIVMSG nobody Response]:")
        print(resp_confirm.strip())
        assert "401" in resp_confirm, "Expected 401 ERR_NOSUCHNICK"
        assert "alice" in resp_confirm, "Server should address the response to 'alice'"

        # 19. Ghost nickname test: abrupt disconnect (Client 3 connects and closes without QUIT)
        print("\n--- Ghost nickname test: Charlie socket closed abruptly without QUIT ---")
        s_charlie = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_charlie.connect(("localhost", port))
        s_charlie.setblocking(False)
        s_charlie.sendall(f"PASS {password}\r\nNICK charlie\r\nUSER charlie 0 * :Charlie Real\r\n".encode())
        time.sleep(0.1)
        read_until_eof(s_charlie)

        s_charlie.close()
        s_charlie = None
        time.sleep(0.5)

        # Bob switches to 'charlie'
        print("\n--- Bob acquires released nickname 'charlie' after abrupt disconnect ---")
        s_bob.sendall(b"NICK charlie\r\n")
        time.sleep(0.1)
        resp_nick_charlie = read_until_eof(s_bob)
        print("[Bob NICK charlie Response]:")
        print(resp_nick_charlie.strip())
        assert "433" not in resp_nick_charlie, "Ghost nickname: server still thinks 'charlie' is in use"

        # 20. [실험] Dave/Eve: 채널 PRIVMSG는 발신자 본인에게 echo되면 안 된다 + PART 브로드캐스트
        print("\n--- [실험] Dave/Eve: 채널 PRIVMSG self-echo 방지 + PART 브로드캐스트 ---")
        s_dave = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_dave.connect(("localhost", port))
        s_dave.setblocking(False)
        s_dave.sendall(f"PASS {password}\r\nNICK dave\r\nUSER dave 0 * :Dave Real\r\n".encode())
        time.sleep(0.1)
        resp_dave = read_until_eof(s_dave)
        assert "001 dave" in resp_dave, "Dave registration failed"

        s_eve = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_eve.connect(("localhost", port))
        s_eve.setblocking(False)
        s_eve.sendall(f"PASS {password}\r\nNICK eve\r\nUSER eve 0 * :Eve Real\r\n".encode())
        time.sleep(0.1)
        resp_eve = read_until_eof(s_eve)
        assert "001 eve" in resp_eve, "Eve registration failed"

        s_dave.sendall(b"JOIN #experiment\r\n")
        s_eve.sendall(b"JOIN #experiment\r\n")
        time.sleep(0.1)
        read_until_eof(s_dave)
        read_until_eof(s_eve)

        s_dave.sendall(b"PRIVMSG #experiment :self echo check\r\n")
        time.sleep(0.1)
        resp_dave_echo = read_until_eof(s_dave)
        resp_eve_msg = read_until_eof(s_eve)
        print("[Dave's own buffer right after his channel PRIVMSG]:")
        print(repr(resp_dave_echo))
        print("[Eve receives channel PRIVMSG]:")
        print(resp_eve_msg.strip())
        assert "PRIVMSG #experiment :self echo check" not in resp_dave_echo, \
            "Server echoed the channel PRIVMSG back to the sender (should only reach other members)"
        assert "dave!dave@" in resp_eve_msg and "PRIVMSG #experiment :self echo check" in resp_eve_msg, \
            "Eve did not receive Dave's channel PRIVMSG"

        s_dave.sendall(b"PART #experiment :bye\r\n")
        time.sleep(0.1)
        resp_eve_part = read_until_eof(s_eve)
        print("[Eve sees Dave's PART]:")
        print(resp_eve_part.strip())
        assert "dave!dave@" in resp_eve_part and "PART #experiment :bye" in resp_eve_part, \
            "Eve did not receive Dave's PART broadcast"
        print("-> self-echo 방지 + PART 브로드캐스트 SUCCESS!")

        # 20.5 [검증] 채널명 대소문자 무시 (Case-insensitive: #CaseChannel vs #casechannel vs #CASECHANNEL)
        print("\n--- [검증] 채널명 대소문자 무시 지원 (#CaseChannel vs #casechannel) ---")
        s_dave.sendall(b"JOIN #CaseChannel\r\n")
        time.sleep(0.1)
        resp_dave_join = read_until_eof(s_dave)
        assert "JOIN #CaseChannel" in resp_dave_join, "Dave failed to join #CaseChannel"

        # Eve가 소문자 #casechannel로 입장 (동일 채널로 연결되어야 함)
        s_eve.sendall(b"JOIN #casechannel\r\n")
        time.sleep(0.1)
        resp_eve_join = read_until_eof(s_eve)
        resp_dave_see_eve = read_until_eof(s_dave)
        print("[Eve JOIN #casechannel response]:")
        print(resp_eve_join.strip())
        print("[Dave sees Eve join]:")
        print(resp_dave_see_eve.strip())
        assert "eve!eve@" in resp_dave_see_eve and "JOIN" in resp_dave_see_eve, \
            "Dave did not see Eve join the same case-insensitive channel"

        # Eve가 대문자 #CASECHANNEL로 PRIVMSG 전송 -> Dave가 수신해야 함
        s_eve.sendall(b"PRIVMSG #CASECHANNEL :Hello case insensitivity!\r\n")
        time.sleep(0.1)
        resp_dave_privmsg = read_until_eof(s_dave)
        print("[Dave receives message from Eve via #CASECHANNEL]:")
        print(resp_dave_privmsg.strip())
        assert "eve!eve@" in resp_dave_privmsg and "PRIVMSG #CASECHANNEL :Hello case insensitivity!" in resp_dave_privmsg, \
            "Dave did not receive PRIVMSG sent to uppercase channel name"

        # Dave가 소문자 #casechannel로 PART -> Eve가 수신해야 함
        s_dave.sendall(b"PART #casechannel :bye\r\n")
        time.sleep(0.1)
        resp_eve_see_part = read_until_eof(s_eve)
        print("[Eve sees Dave PART via #casechannel]:")
        print(resp_eve_see_part.strip())
        assert "dave!dave@" in resp_eve_see_part and "PART" in resp_eve_see_part, \
            "Eve did not see Dave PART with different casing"
        print("-> 채널명 대소문자 무시 (Case-insensitive) SUCCESS!")

        # 21. [실험] 512바이트(RFC1459 2.3 CRLF 포함 라인 한도) 초과 라인 → 응답 없이 즉시 연결 종료
        #     (Client::extractLine, src/client/Client.cpp)
        print("\n--- [실험] 512바이트 초과 라인 전송 시 즉시 연결 종료 확인 ---")
        s_oversize = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_oversize.connect(("localhost", port))
        s_oversize.setblocking(False)
        s_oversize.sendall(b"A" * 600)  # \r\n 없이 512바이트를 넘는 데이터만 전송
        time.sleep(0.2)
        resp_oversize = read_until_eof(s_oversize)
        print(f"[Oversized-line response]: {resp_oversize!r} (빈 문자열이어야 함)")
        assert resp_oversize == "", "Server should not reply to an oversized (>512 byte) line"
        try:
            trailing = s_oversize.recv(16)
            assert trailing == b"", "Server kept the connection open instead of closing it"
        except BlockingIOError:
            raise AssertionError("Server did not disconnect the client after an oversized (>512 byte) line")
        s_oversize.close()
        s_oversize = None
        print("-> 512바이트 초과 라인 처리 SUCCESS!")

        # 22. [실험] out-buffer 64KB(SendQ) 한도 초과 시 강제 종료
        #     (Client::appendToOutBuffer, src/client/Client.cpp) — sink는 절대 recv하지 않는 "느린 리더" 역할
        print("\n--- [실험] 대상 클라이언트가 읽지 않는 상태에서 채널에 메시지를 대량 전송해 SendQ 초과 유도 ---")
        s_flooder = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_flooder.connect(("localhost", port))
        s_flooder.setblocking(False)
        s_flooder.sendall(f"PASS {password}\r\nNICK flooder\r\nUSER flooder 0 * :Flooder\r\n".encode())
        time.sleep(0.1)
        resp_flooder = read_until_eof(s_flooder)
        assert "001 flooder" in resp_flooder, "Flooder registration failed"

        s_sink = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        # 커널 수신 윈도우를 최소치로 줄여, sink가 안 읽는 동안 서버 쪽 로컬 송신 버퍼가
        # 빨리 막히도록(backpressure) 유도한다 — 그래야 서버 소켓 기본 버퍼 크기(수백KB~)에
        # 가려지지 않고 "안 읽는 클라이언트"에 대한 서버의 방어 로직을 현실적인 시간 안에 관찰할 수 있다.
        s_sink.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 1024)
        s_sink.connect(("localhost", port))
        s_sink.setblocking(False)
        s_sink.sendall(f"PASS {password}\r\nNICK sink\r\nUSER sink 0 * :Sink\r\n".encode())
        time.sleep(0.1)
        resp_sink = read_until_eof(s_sink)
        assert "001 sink" in resp_sink, "Sink registration failed"

        s_flooder.sendall(b"JOIN #flood\r\n")
        s_sink.sendall(b"JOIN #flood\r\n")
        time.sleep(0.1)
        read_until_eof(s_flooder)
        read_until_eof(s_sink)  # sink의 마지막 수신 - 이후로는 절대 recv하지 않는다 (버퍼를 쌓기 위함)

        # 브로드캐스트 1건당 out-buffer에는 최대 512바이트(510B 본문 + CRLF)만 쌓인다(Client::appendToOutBuffer의
        # kMaxLineBody 절단 때문). localhost는 RTT가 거의 0이라 커널 버퍼 몇백KB 정도로는 체감되는
        # backpressure가 거의 없어(즉시 다 흘려보내짐), 실측상 sink가 전혀 안 읽는 상태에서도 서버가 결국
        # 막히게 하려면 총량을 ~3MB(480B 메시지 6000개) 수준까지 올려야 한다(1MB로는 재현되지 않았음).
        flood_payload = ("PRIVMSG #flood :" + ("X" * 480) + "\r\n") * 6000
        try:
            s_flooder.sendall(flood_payload.encode())
        except BlockingIOError:
            # 논블로킹 소켓의 로컬 송신 버퍼가 이미 가득 찼다는 뜻 — 그 자체로 서버 쪽
            # backpressure를 유발하기에 충분한 양이 이미 전달된 것이므로 계속 진행한다.
            pass

        # sink가 읽지 않는 상태가 계속되면, 서버는 (a) SendQ 64KB 한도 초과로 스스로 끊거나
        # (b) 로컬 송신 버퍼가 막혀 send()가 실패하는 즉시 끊거나, 둘 중 하나로 결국 sink 연결을
        # 정리해야 한다 — black box 관점에서는 어느 경로든 "느린 리더가 서버를 무한정 막지 못한다"만 확인한다.
        deadline = time.monotonic() + 5.0
        disconnected = False
        while time.monotonic() < deadline and not disconnected:
            try:
                chunk = s_sink.recv(65536)
                if chunk == b"":
                    disconnected = True
                    break
            except BlockingIOError:
                time.sleep(0.05)
        print(f"[Sink] 느린 리더 강제 종료 감지: {disconnected}")
        assert disconnected, \
            "서버가 out-buffer가 쌓인(느린 리더) 클라이언트를 끊지 않았다 (Client::appendToOutBuffer SendQ 방어 미동작)"
        s_flooder.close()
        s_flooder = None
        s_sink.close()
        s_sink = None
        print("-> 느린 리더(non-draining client) 강제 종료 SUCCESS!")

        # Cleanup sockets
        s_dave.close()
        s_eve.close()
        s_bob.close()
        print("\n=== INTEGRATION TEST PASSED SUCCESSFULLY ===")

    finally:
        # 테스트 본문에서 일부 소켓은 이미 닫혔을 수 있으므로 각각 안전하게 정리한다
        for sock in (s_alice, s_bob, s_dave, s_eve, s_oversize, s_flooder, s_sink):
            if sock is not None:
                try:
                    sock.close()
                except Exception:
                    pass
        server_process.terminate()
        server_process.wait()

if __name__ == "__main__":
    main()
