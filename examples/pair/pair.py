from serial import Serial
from serial.tools.list_ports import comports
import argparse
import struct
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
                break
        else:
            raise Exception('ESP32-S3 COM port not found')

    ser = Serial(port_name)

    # ssid = input('Enter SSID: ')
    # password = input('Enter password: ')
    ssid = "broderio"
    password = "i<3robots!"
    print("Pairing device with SSID: ", ssid, " and password: ", password)

    ssid_bytes = (ssid + '\0' * 9)[:9].encode()
    password_bytes = (password + '\0' * 16)[:16].encode()
    pair_config = struct.pack('9s16s', ssid_bytes, password_bytes)

    ack_send = bytes([0xFF])

    ser.write(ack_send)
    while (True):
        ack = ser.read(1)
        if (ack[0] == 0xFF):
            ser.write(pair_config)
        else:
            ack += ser.read(8)
            print("Device already paired! SSID: ", ack.decode('utf-8').rstrip('\0'))
            print("Press the pair button to re-pair the device")
            ser.read(1)
            ser.write(pair_config)
        
        device_config = ser.read(25)
        if (pair_config == device_config):
            print("Device paired successfully!")
            break
        else:
            print("Device pairing failed! Retrying ...")
            ser.write(ack_send)

if (__name__ == '__main__'):
    main()
    