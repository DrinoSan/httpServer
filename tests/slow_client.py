#!/usr/bin/env python3
"""
Simulate a slow client sending an HTTP request body in multiple parts,
with delays between each part. Useful for exercising kqueue read-readiness
and idle/keep-alive timeout handling on partial requests.

Usage:
    python3 slow_client.py [host] [port] [delay_seconds]

Example:
    python3 slow_client.py 127.0.0.1 8080 2
"""

import socket
import sys
import time

HOST = sys.argv[1] if len(sys.argv) > 1 else "127.0.0.1"
PORT = int(sys.argv[2]) if len(sys.argv) > 2 else 8080
DELAY = float(sys.argv[3]) if len(sys.argv) > 3 else 2.0

# Split into 3 body parts so the server has to reassemble a partial
# request across several read() calls.
BODY_PARTS = [b"foo=bar&", b"baz=qux&", b"last=1"]
BODY = b"".join(BODY_PARTS)

HEADERS = (
    "POST / HTTP/1.1\r\n"
    f"Host: {HOST}\r\n"
    "Content-Type: application/x-www-form-urlencoded\r\n"
    f"Content-Length: {len(BODY)}\r\n"
    "Connection: keep-alive\r\n"
    "\r\n"
).encode()


def send_part(sock, data, label):
    print(f"[{time.strftime('%H:%M:%S')}] sending {label!r} ({len(data)} bytes)")
    sock.sendall(data)


def main():
    sock = socket.create_connection((HOST, PORT))
    sock.settimeout(30)

    try:
        send_part(sock, HEADERS, "headers")
        time.sleep(DELAY)

        for i, part in enumerate(BODY_PARTS, 1):
            send_part(sock, part, f"body part {i}/{len(BODY_PARTS)}")
            if i != len(BODY_PARTS):
                time.sleep(DELAY)

        print("body fully sent, waiting for response...")
        response = b""
        try:
            while True:
                chunk = sock.recv(4096)
                if not chunk:
                    break
                response += chunk
                # Stop once we've plausibly got a full response for a simple test.
                if b"\r\n\r\n" in response:
                    break
        except socket.timeout:
            print("timed out waiting for response")

        print("---- response ----")
        print(response.decode(errors="replace"))
    finally:
        sock.close()


if __name__ == "__main__":
    main()
