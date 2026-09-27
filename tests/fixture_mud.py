import socket
import sys
import threading
import time

IAC, SE, SB, WILL, WONT, DO, DONT = 255, 240, 250, 251, 252, 253, 254
ECHO, SGA, TTYPE, NAWS, MCCP2 = 1, 3, 24, 31, 86

BANNER = (
    "\x1b[0;36mCoffeeMUD v5.11\x1b[0m\r\n"
    "\x1b[1;33mWelcome to a world that keeps running without you.\x1b[0m\r\n"
    "This line is deliberately longer than sixty-four columns so the wrap can be checked.\r\n"
    "Col1\tCol2\tCol3\r\n"
    "\x07"
    "By what name do you wish to be known?\r\n"
)

def serve(port, result):
    srv = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    srv.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    srv.bind(("127.0.0.1", port))
    srv.listen(1)
    conn, _ = srv.accept()
    conn.settimeout(3.0)

    conn.sendall(bytes([IAC, DO, TTYPE, IAC, DO, NAWS, IAC, WILL, MCCP2, IAC, WILL, SGA]))

    got = b""
    deadline = time.time() + 2.0
    while time.time() < deadline:
        try:
            chunk = conn.recv(4096)
        except socket.timeout:
            break
        if not chunk:
            break
        got += chunk
        if bytes([IAC, SB, NAWS]) in got and bytes([IAC, WILL, TTYPE]) in got:
            break

    conn.sendall(bytes([IAC, SB, TTYPE, 1, IAC, SE]))
    conn.sendall(BANNER.encode("ascii"))
    conn.sendall(bytes([IAC, WILL, ECHO]))

    deadline = time.time() + 2.0
    while time.time() < deadline:
        try:
            chunk = conn.recv(4096)
        except socket.timeout:
            continue
        if not chunk:
            break
        got += chunk
        if b"DUMB" in got and bytes([IAC, DO, ECHO]) in got:
            break

    result["client_sent"] = got
    try:
        conn.close()
    finally:
        srv.close()

def main():
    port = int(sys.argv[1]) if len(sys.argv) > 1 else 5599
    result = {}
    t = threading.Thread(target=serve, args=(port, result))
    t.start()
    t.join(15)

    sent = result.get("client_sent", b"")
    failures = []

    if bytes([IAC, WILL, NAWS]) not in sent:
        failures.append("client never offered NAWS")
    if bytes([IAC, SB, NAWS, 0, 80, 0, 25, IAC, SE]) not in sent:
        failures.append("client did not report 80x25")
    if bytes([IAC, WILL, TTYPE]) not in sent:
        failures.append("client did not accept TERMINAL-TYPE")
    if b"DUMB" not in sent:
        failures.append("client did not report DUMB")
    if bytes([IAC, DONT, MCCP2]) not in sent:
        failures.append("client did not refuse MCCP2")
    if bytes([IAC, DO, ECHO]) not in sent:
        failures.append("client did not take server echo")

    for f in failures:
        print("FAIL " + f)
    if failures:
        print("client sent: " + " ".join("%02x" % b for b in sent))
        return 1
    print("ok   fixture_mud: client negotiated 80x25, DUMB, no MCCP, echo taken")
    return 0

if __name__ == "__main__":
    sys.exit(main())
