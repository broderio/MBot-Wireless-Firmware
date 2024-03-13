import struct
from serial import Serial
from serial.tools.list_ports import comports
from collections import deque
import threading
import time
from math import fabs
import matplotlib.pyplot as plt
from matplotlib.collections import LineCollection
import numpy as np
import argparse
from numba import jit

class SerialPose2D:
    byte_size = 8 + 4 * 3
    def __init__(self):
        self.utime: int = 0
        self.x: float = 0.0
        self.y: float = 0.0
        self.theta: float = 0.0
    
    def decode(self, data: bytes):
        self.utime, self.x, self.y, self.theta = struct.unpack('qfff', data)

    def encode(self) -> bytes:
        return struct.pack('qfff', self.utime, self.x, self.y, self.theta)

class SerialTwist2D:
    byte_size = 8 + 4 * 3
    def __init__(self):
        self.utime: int = 0
        self.vx: float = 0.0
        self.vy: float = 0.0
        self.wz: float = 0.0

    def decode(self, data: bytes):
        self.utime, self.vx, self.vy, self.wz = struct.unpack('qfff', data)

    def encode(self) -> bytes:
        return struct.pack('qfff', self.utime, self.vx, self.vy, self.wz)
    
class SerialLidarScan:
    byte_size = 8 + 2 * 360
    def __init__(self):
        self.utime: int = 0
        self.ranges: list = [0] * 360
    
    def decode(self, data: bytes):
        self.utime = struct.unpack('q', data[:8])[0]
        self.ranges = struct.unpack('H'*360, data[8:8+720])

    def encode(self) -> bytes:
        return struct.pack('q' + 'H'*360, self.utime, *self.ranges)
    
def checksum(addends: bytes) -> int:
    return 255 - (sum(addends) % 256)

def create_message(topic: int, robot_id: int, msg: bytes) -> bytes:
    SYNC_FLAG = 0xff
    VERSION_FLAG = 0xfe
    len_msg = len(msg)
    total_len_msg = len_msg + 8
    
    # Create packet
    pkt = bytearray(len_msg + 8 + 4)  # 8 extra bytes for the header and footer, 4 extra bytes for second header
    pkt[0] = SYNC_FLAG
    pkt[1] = robot_id
    pkt[2] = total_len_msg & 0xFF  # message length lower 8 bits
    pkt[3] = total_len_msg >> 8  # message length higher 8 bits
    
    pkt[4] = SYNC_FLAG
    pkt[5] = VERSION_FLAG
    pkt[6] = len_msg & 0xFF  # message length lower 8 bits
    pkt[7] = len_msg >> 8  # message length higher 8 bits
    pkt[8] = checksum(pkt[6:8])  # checksum over message length
    pkt[9] = topic & 0xFF  # message topic lower 8 bits
    pkt[10] = topic >> 8  # message topic higher 8 bits

    pkt[11:len_msg+11] = msg  # copy message bytes

    # Create array for the checksum over topic and message content
    cs2_addends = bytearray(len_msg + 2)
    cs2_addends[0] = pkt[9]
    cs2_addends[1] = pkt[10]
    cs2_addends[2:] = msg

    pkt[len_msg+11] = checksum(cs2_addends)  # checksum over message data and topic
    return pkt
    
def parse_message(data: bytes) -> (int, bytes) or None:
    if (data[0] != 0xff or data[1] != 0xfe):
        return None

    header = data[0:7]
    sync = header[0]
    version = header[1]
    msg_len = struct.unpack('<H', header[2:4])[0]
    cs1 = header[4]
    topic = struct.unpack('<H', header[5:7])[0]
    msg = data[7:7+msg_len]
    cs2 = data[7+msg_len]

    return topic, msg

class SerialReader:
    def __init__(self, ser: Serial):
        self.ser = ser
        self.leftover = bytearray()

    def get_packet_serial(self) -> (int, bytes) or None:
        # Read in larger chunks until we find the first sync byte
        chunk = self.leftover
        sync_index = chunk.find(b'\xff')
        while (sync_index == -1 or ((len(chunk) - sync_index) < 4)):
            chunk += self.ser.read(1024)
            sync_index = chunk.find(b'\xff')

        # Parse header
        header = chunk[sync_index:sync_index+4]

        # First byte is robot ID
        robot_id = struct.unpack('B', header[1:2])[0]

        # Second byte is lower byte of msg length, third byte is higher byte
        msg_length = struct.unpack('<H', header[2:4])[0]

        # Read the rest of the message
        msg = chunk[sync_index+4:sync_index+4+msg_length]
        while len(msg) < msg_length:
            msg += self.ser.read(msg_length - len(msg))

        # Update leftover
        self.leftover = chunk[sync_index+4+msg_length:]

        # Return reassembled packet
        return robot_id, msg

class VelocityCommander:
    def __init__(self, ser: Serial):
        self.ser = ser
        self.running = False
        self.thread = None
        self.vx = 0.0

    def start(self):
        self.running = True
        self.thread = threading.Thread(target=self.send_velocity_commands, daemon=True)
        self.thread.start()

    def stop(self):
        self.running = False
        if self.thread is not None:
            self.thread.join()

    def send_velocity_commands(self):
        delta = 0.1
        while self.running:
            twist = SerialTwist2D()
            twist.vx = self.vx
            msg = twist.encode()
            pkt = create_message(214, 0, msg)  # Assuming 200 is the topic for velocity commands 
            self.ser.write(pkt)
            time.sleep(1)  # Wait for 1 second

            # Oscillate vx between -0.5 and 0.5
            self.vx += delta
            if fabs(self.vx) > 0.4:
                delta *= -1

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
   
    reader = SerialReader(ser)
    # commander = VelocityCommander(ser)
    # commander.start()

    plt.ion()
    fig = plt.figure(facecolor='#989C97')  # Set the background color
    ax = fig.add_subplot(111, projection='polar', frame_on=False)  # Remove the frame
    
    ax.grid(False)  # Remove the grid
    ax.set_rmax(4)
    ax.set_xticklabels([])  # Remove the labels on the radius
    ax.set_yticklabels([])  # Remove the labels on the radius
    ax.set_rticks([])  # Remove the ticks on the radius

    angles = np.array([np.radians(x) for x in range(360)])[::-1]
    lines = LineCollection([], colors='#FFCB05', linewidths=0.5)  # Initialize an empty LineCollection with thinner lines
    ax.add_collection(lines)  # Add the LineCollection to the plot
    obstacle_line, = ax.plot([], [], color='#00274C')

    line_segments = np.zeros((360, 2, 2))

    @jit(nopython=True)
    def update_line_segments(angles, distances, line_segments):
        line_segments[:, 1, 0] = angles
        line_segments[:, 1, 1] = distances

    while True:
        packet = reader.get_packet_serial()
        if packet is not None:
            robot_id, msg = packet
            if robot_id == 0:
                out = parse_message(msg)
                if out is not None:
                    topic, pkt = out
                    if (topic == 210):
                        pose = SerialPose2D()
                        pose.decode(pkt)
                        # print(f'\033c Pose: {round(pose.x, 4)}, {round(pose.y, 4)}, {round(pose.theta, 4)}', end='', flush=True)
                    elif (topic == 240):
                        scan = SerialLidarScan()
                        scan.decode(pkt)
                        distances = np.array(scan.ranges, dtype=np.float64)  # Create a float array
                        distances /= 1000 
                        
                        update_line_segments(angles, distances, line_segments)  # Update line segments with Numba-compiled function
                        lines.set_segments(line_segments)
                        obstacle_line.set_data(angles, distances)
                        fig.canvas.draw()
                        fig.canvas.flush_events()


if __name__ == '__main__':
    main()                