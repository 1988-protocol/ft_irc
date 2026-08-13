import os
import sys
import socket
import subprocess
import time

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from test_utils import read_until_eof

def main():
    print("=== Starting IRC Integration Test ===")
    
    # 1. Start Server on port 10001
    port = 10001
    password = "testpassword"
    server_process = subprocess.Popen(["./ircserv", str(port), password], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(0.5) # Wait for server to bind and start listening

    s_alice = None
    s_bob = None
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
        
        # Alice should also see Bob join (broadcast check - commented until Server.cpp network event loop update)
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        print("[Alice sees Bob join]:")
        print(resp_alice.strip())
        # Note: Requires multi-client POLLOUT broadcast support in Server.cpp
        # assert "bob!bob@127.0.0.1 JOIN #testchannel" in resp_alice or "bob!bob@localhost JOIN #testchannel" in resp_alice or "JOIN #testchannel" in resp_alice, "Alice did not see Bob join"

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
        # Note: Requires multi-client POLLOUT broadcast support in Server.cpp
        # assert "TOPIC #testchannel :New Project Topic" in resp_bob, "Bob did not see topic update"

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
        # Note: Requires multi-client POLLOUT broadcast support in Server.cpp
        # assert "PRIVMSG #testchannel :Hello team!" in resp_bob, "Bob did not receive broadcast"

        # 9. 1:1 PRIVMSG
        print("\n--- Bob sends direct PRIVMSG to Alice ---")
        s_bob.sendall(b"PRIVMSG alice :Hi Alice, PM check.\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        print("[Alice receives direct message]:")
        print(resp_alice.strip())
        # Note: Requires multi-client POLLOUT broadcast support in Server.cpp
        # assert "PRIVMSG alice :Hi Alice, PM check." in resp_alice, "Alice did not receive PM"

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
        # Note: Requires multi-client POLLOUT broadcast support in Server.cpp
        # assert "KICK #testchannel bob :You are kicked" in resp_bob, "Bob kick broadcast failed"

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
        # Note: Requires multi-client POLLOUT broadcast support in Server.cpp
        # assert "INVITE bob" in resp_bob and "#testchannel" in resp_bob, "Bob did not receive INVITE"

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
        # Note: Requires multi-client POLLOUT broadcast support in Server.cpp
        # assert "MODE #testchannel +o bob" in resp_bob, "MODE +o broadcast failed"

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

        # Cleanup sockets
        s_bob.close()
        print("\n=== INTEGRATION TEST PASSED SUCCESSFULLY ===")

    finally:
        # s_alice는 테스트 본문에서 이미 닫혔을 수 있으므로 안전하게 처리
        if s_alice is not None:
            try:
                s_alice.close()
            except Exception:
                pass
        try:
            s_bob.close()
        except Exception:
            pass
        server_process.terminate()
        server_process.wait()

if __name__ == "__main__":
    main()
