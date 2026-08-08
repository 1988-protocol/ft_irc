import socket
import time

def read_until_eof(sock, timeout=0.5, idle_timeout=0.08):
    """
    비차단(non-blocking) 소켓에서 지정된 timeout 및 idle_timeout 기준으로 데이터를 누적 수신합니다.
    - 첫 데이터 수신 후 즉시 종료하지 않고, idle_timeout(0.08초) 동안 추가 데이터가 없을 때까지 버퍼를 완전히 비웁니다.
    - TCP 패킷 분할(Segmentation) 및 서버의 다중 응답 청크를 안전하게 모두 수신합니다.
    - 상대 소켓이 닫히면(EOF, chunk == '') 즉시 수신을 종료하고 누적된 데이터를 반환합니다.
    """
    sock.setblocking(False)
    data = ""
    start_time = time.time()
    last_recv_time = None

    while time.time() - start_time < timeout:
        try:
            chunk = sock.recv(4096).decode('utf-8')
            if chunk:
                data += chunk
                last_recv_time = time.time()
            else:
                # 상대방 소켓이 종료(EOF)된 경우
                break
        except socket.error:
            pass

        # 데이터를 최소 1회 수신했고, 마지막 수신 시점 이후 idle_timeout 경과 시 완료 처리
        if last_recv_time is not None and (time.time() - last_recv_time >= idle_timeout):
            break

        time.sleep(0.01)

    return data
