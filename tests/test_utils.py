import errno
import socket
import time

def read_until_eof(sock, timeout=0.5, idle_timeout=0.08):
    """
    비차단(non-blocking) 소켓에서 지정된 timeout 및 idle_timeout 기준으로 데이터를 누적 수신합니다.
    - time.monotonic() (단조 시계):
      시스템 시계(NTP 시간 동기화, 사용자의 시간 변경 등)의 영향을 받지 않고 오직 앞으로만 일정하게 증가하는 시계입니다.
      time.time()과 달리 시간이 뒤로 가거나 건너뛰지 않아 타임아웃 및 경과 시간 측정의 표준으로 사용됩니다.
    - 바이트(bytes) 단위로 먼저 누적한 뒤 마지막에 UTF-8로 디코드하여 멀티바이트 문자 분할(UnicodeDecodeError)을 방지합니다.
    - 비차단 대기 오류(BlockingIOError / EAGAIN / EWOULDBLOCK)만 안전하게 대기하고,
      ConnectionResetError나 EBADF 등 실제 소켓 장애는 다시 발생(re-raise)시킵니다.
    - 첫 데이터 수신 후 즉시 종료하지 않고, idle_timeout(0.08초) 동안 추가 데이터가 없을 때까지 버퍼를 완전히 비웁니다.
    - 상대 소켓이 닫히면(EOF, chunk == b'') 즉시 수신을 종료하고 누적된 데이터를 반환합니다.
    """
    sock.setblocking(False)
    raw_bytes = bytearray()
    # time.monotonic(): 시스템 시계 변경에 영향을 받지 않는 단조 시계로 시작 시각 기록
    start_time = time.monotonic()
    last_recv_time = None

    while time.monotonic() - start_time < timeout:
        try:
            chunk = sock.recv(4096)
            if chunk:
                raw_bytes.extend(chunk)
                last_recv_time = time.monotonic()
            else:
                # 상대방 소켓이 종료(EOF / FIN 수신)된 경우
                break
        except BlockingIOError:
            # 아직 버퍼에 읽을 데이터가 없는 논블로킹 대기 상태
            pass
        except socket.error as e:
            # EAGAIN / EWOULDBLOCK (11) 외의 ConnectionResetError(ECONNRESET)나 EBADF 등은 다시 발생시킴
            if e.errno in (errno.EAGAIN, errno.EWOULDBLOCK):
                pass
            else:
                raise

        # 데이터를 최소 1회 수신했고, 마지막 수신 시점 이후 idle_timeout 경과 시 완료 처리
        if last_recv_time is not None and (time.monotonic() - last_recv_time >= idle_timeout):
            break

        time.sleep(0.01)

    return raw_bytes.decode('utf-8', errors='replace')


