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
        
        s_bob.sendall(f"PASS {password}\r\nNICK bob\r\nUSER bob 0 * :Bob Real\r\n".encode())
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
        
        # Alice should also see Bob join
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
        assert "MODE #testchannel +l 2" in resp_alice, "MODE +l broadcast failed"

        # 17. Alice QUITs
        print("\n--- Alice QUITs ---")
        s_alice.sendall(b"QUIT :Done testing\r\n")
        time.sleep(0.1)
        resp_alice = read_until_eof(s_alice)
        print("[Alice sees QUIT response]:")
        print(resp_alice.strip())
        assert "ERROR :Closing Link: Done testing" in resp_alice, "Alice QUIT failed"

        # Cleanup sockets
        s_alice.close()
        s_bob.close()
        print("\n=== INTEGRATION TEST PASSED SUCCESSFULLY ===")

    finally:
        server_process.terminate()
        server_process.wait()

if __name__ == "__main__":
    main()
