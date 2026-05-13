import socket
import os
import struct
import sys

# Get current UID
uid = os.getuid()
socket_path = f"/tmp/horizon-keyring-test.socket"

if os.path.exists(socket_path):
    os.remove(socket_path)

server = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
server.bind(socket_path)
server.listen(1)

print(f"Mock Keyring Daemon listening on {socket_path}")
print("Press Ctrl+C to stop")

try:
    while True:
        conn, addr = server.accept()
        print("Connection received!")
        
        # Protocol: [length(uint32_t)] + [password]
        data = conn.recv(4)
        if not data:
            conn.close()
            continue
            
        length = struct.unpack("I", data)[0]
        password = conn.recv(length).decode('utf-8')
        
        print(f"CAPTURED PASSWORD: {password}")
        conn.close()
except KeyboardInterrupt:
    print("\nStopping...")
finally:
    os.remove(socket_path)
