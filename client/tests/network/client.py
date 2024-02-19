import socket

def create_tcp_client(ip: str, port: int) -> socket.socket:
    # Create a TCP/IP socket
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    # Connect the socket to the server
    server_address = (ip, port)
    print(f"Connecting to {server_address}")
    sock.connect(server_address)

    return sock

# Use the function to create a client and connect to the server
client_socket = create_tcp_client('10.0.0.2', 1234)
client_socket.sendall(b'Hello, server!')