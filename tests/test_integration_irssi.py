import socket
import subprocess
import time
import sys

def read_until_eof(sock, timeout=0.5):
    sock.setblocking(False)
    data = ""
    start_time = time.time()
    while time.time() - start_time < timeout:
        try:
            chunk = sock.recv(4096).decode('utf-8')
            if chunk:
                data += chunk
            elif data:
                break
        except socket.error:
            pass
        if data:
            break
        time.sleep(0.05)
    return data

def main():
    print("=== Starting Irssi Client Simulation Integration Test ===")
    
    # 1. Start Server on port 10002
    port = 10002
    password = "testpassword"
    server_process = subprocess.Popen(["./ircserv", str(port), password], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    time.sleep(0.5) # Wait for server to bind and start listening

    try:
        # 2. Irssi Alice Registration (Pipelined packet test)
        # Irssi sends CAP LS 302, PASS, NICK, USER all in a single TCP packet on connection.
        print("\n--- 1. Irssi Pipelined Registration (CAP LS + PASS + NICK + USER) ---")
        s_alice = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_alice.connect(("localhost", port))
        s_alice.setblocking(False)
        
        irssi_alice_handshake = f"CAP LS 302\r\nPASS {password}\r\nNICK ir_alice\r\nUSER ir_alice 0 * :Irssi User Alice\r\n"
        s_alice.sendall(irssi_alice_handshake.encode())
        time.sleep(0.2)
        resp_alice = read_until_eof(s_alice)
        print("[Irssi Alice Response]:")
        print(resp_alice.strip())
        assert "001 ir_alice" in resp_alice, "Irssi Alice registration failed (001 not found)"
        print("-> Irssi pipelined handshake registration SUCCESS!")

        # 3. Irssi Automatic User Mode setting (MODE ir_alice +i)
        print("\n--- 2. Irssi Auto User Mode (MODE ir_alice +i) ---")
        s_alice.sendall(b"MODE ir_alice +i\r\n")
        time.sleep(0.1)
        resp_mode = read_until_eof(s_alice)
        print("[Irssi User Mode Response]:")
        print(resp_mode.strip())

        # 4. Irssi Bob Registration
        print("\n--- 3. Irssi Bob Pipelined Registration ---")
        s_bob = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s_bob.connect(("localhost", port))
        s_bob.setblocking(False)
        
        irssi_bob_handshake = f"CAP LS 302\r\nPASS {password}\r\nNICK ir_bob\r\nUSER ir_bob 0 * :Irssi User Bob\r\n"
        s_bob.sendall(irssi_bob_handshake.encode())
        time.sleep(0.2)
        resp_bob = read_until_eof(s_bob)
        print("[Irssi Bob Response]:")
        print(resp_bob.strip())
        assert "001 ir_bob" in resp_bob, "Irssi Bob registration failed (001 not found)"

        # 5. Irssi PING / PONG Keepalive simulation
        print("\n--- 4. Irssi PING / PONG Simulation ---")
        ping_token = "1698765432"
        s_alice.sendall(f"PING :{ping_token}\r\n".encode())
        time.sleep(0.1)
        resp_ping = read_until_eof(s_alice)
        print("[PING Response]:")
        print(resp_ping.strip())
        assert "PONG" in resp_ping and ping_token in resp_ping, "PING/PONG failed"
        print("-> Irssi PING/PONG keepalive SUCCESS!")

        # 6. Irssi Channel JOIN and Automatic Channel Mode / Member Queries
        print("\n--- 5. Irssi Channel JOIN & Auto Queries ---")
        s_alice.sendall(b"JOIN #irssichan\r\nMODE #irssichan\r\n")
        time.sleep(0.2)
        resp_join = read_until_eof(s_alice)
        print("[Alice JOIN #irssichan Response]:")
        print(resp_join.strip())
        assert "JOIN #irssichan" in resp_join, "Irssi JOIN failed"
        print("-> Irssi Channel JOIN & Query SUCCESS!")

        # 7. Irssi 1:1 PRIVMSG
        print("\n--- 6. Irssi PRIVMSG Communication ---")
        s_bob.sendall(b"PRIVMSG ir_alice :Hello from Irssi Bob!\r\n")
        time.sleep(0.1)
        resp_pm = read_until_eof(s_alice)
        print("[Alice received PRIVMSG]:")
        print(resp_pm.strip())
        assert "PRIVMSG ir_alice :Hello from Irssi Bob!" in resp_pm, "Irssi PRIVMSG failed"
        print("-> Irssi PRIVMSG SUCCESS!")

        # Cleanup sockets
        s_alice.close()
        s_bob.close()
        print("\n=== IRSSI INTEGRATION TEST PASSED SUCCESSFULLY ===")

    finally:
        server_process.terminate()
        server_process.wait()

if __name__ == "__main__":
    main()
