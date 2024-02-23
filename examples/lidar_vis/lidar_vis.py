from serial import Serial
from serial.tools.list_ports import comports
import threading
import argparse
import matplotlib.pyplot as plt
import numpy as np
import time

distances = np.array([0] * 360)
angles = np.array([np.radians(x) for x in range(360)])[::-1]

def serial_thread(ser: Serial):
    global distances
    start_time = time.time()
    num_scans = 0
    while True:
        if ser.read(2) == b'\xff\xff':
            buffer = ser.read(360 * 2)
            distances = np.frombuffer(buffer, dtype=np.uint16) / 1000

            num_scans += 1
            elapsed_time = time.time() - start_time
            avg_scan_frequency = num_scans / elapsed_time
            print(f"\r{avg_scan_frequency} Hz, Buffer size: {ser.in_waiting}", end='')


def main():
    parser = argparse.ArgumentParser(description='Monitor serial port')
    parser.add_argument('--port', type=str, help='Serial port name')
    args = parser.parse_args()

    if args.port:
        port_name = args.port
    else:
        ports = comports()

        for port, _, hwid in ports:
            if '303A:1001' in hwid or '10C4:EA60' in hwid or '303A:4001' in hwid:
                port_name = port
                break
        else:
            raise Exception('ESP32-S3 COM port not found')

    ser = Serial(port_name)
   
    serial_handle = threading.Thread(target=serial_thread, args=(ser,), daemon=True)
    serial_handle.start()

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