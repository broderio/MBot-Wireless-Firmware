from serial import Serial
from serial.tools.list_ports import comports
import argparse
from time import sleep

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
                print(f'Found ESP32-S3 COM port: {port_name}')
                break
        else:
            raise Exception('ESP32-S3 COM port not found')

    ser = Serial(port_name)

    while (True):
        ser.write(b'Hello, World!\n')
        print(ser.readline())
        sleep(1)

        
if __name__ == "__main__":
    main()