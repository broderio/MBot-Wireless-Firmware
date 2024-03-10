import socket
import threading
import argparse
import matplotlib.pyplot as plt
import numpy as np
import time

DENSITY = 1

distances = np.array([0] * DENSITY * 360)
angles = np.array([np.radians(x) for x in np.arange(0, 360, 1 / DENSITY)])[::-1]

def socket_thread(sock: socket.socket):
    global distances
    start_time = time.time()
    num_scans = 0
    while True:
        if sock.recv(2) == b'\xff\xff':
            buffer = bytearray()
            while (len(buffer) < DENSITY * 360 * 2):
                buffer += sock.recv(DENSITY * 360 * 2 - len(buffer))
            distances = np.frombuffer(buffer, dtype=np.uint16) / 1000

            num_scans += 1
            elapsed_time = time.time() - start_time
            avg_scan_frequency = num_scans / elapsed_time
            print(f"\r{avg_scan_frequency} Hz", end='')


def main():
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server_address = ("10.0.0.2", 1234)
    print(f"Connecting to {server_address}")
    sock.connect(server_address)
   
    socket_handle = threading.Thread(target=socket_thread, args=(sock,), daemon=True)
    socket_handle.start()

    # Initialize a plot
    plt.ion()
    fig = plt.figure()
    ax = fig.add_subplot(111, projection='polar')  # Create a polar plot
    while True:
        ax.clear()
        index = distances < 12
        ax.scatter(angles[index], distances[index])  # Plot data in polar coordinates
        plt.draw()
        plt.pause(0.1)
        
if __name__ == "__main__":
    main()