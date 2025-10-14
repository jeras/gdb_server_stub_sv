import socket
import os

# format packet
def format_packet (text: str) -> bytes:
    packet = f"${text}#{sum(text.encode())%0x100:02x}"
#    print("FORMAT: {} [{}]", packet, len(packet))
#    print("text.encode() = {}", text.encode())
    return packet.encode()

def scan_packet (packet: bytes) -> str:
    packet_payload = packet[1:-3]
    packet_checksum = packet[-2:-1]
    return packet_payload.decode()

# Set the path for the Unix socket
socket_path = 'unix-socket'

# Create the Unix socket client
client = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

# Connect to the server
client.connect(socket_path)

# Send a message to the server
message = 'Hello from client to server!'
client.sendall(format_packet(message))

# Receive a response from the server
response = client.recv(1024)
print(f'Client received response from server: {scan_packet(response)}')

# Close the connection
client.close()
