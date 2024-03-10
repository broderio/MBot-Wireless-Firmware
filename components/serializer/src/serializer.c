#include <stdio.h>

#include "esp_log.h"

#include "serializer.h"
#include "lcm_types.h"

uint8_t checksum(uint8_t* addends, int len) {
    int sum = 0;
    for (int i = 0; i < len; i++) {
        sum += addends[i];
    }
    return 255 - ((sum) % 256);
}

void pose2D_t_deserialize(uint8_t* src, serial_pose2D_t* dest) {
    memcpy(dest, src, sizeof(serial_pose2D_t));
}

void pose2D_t_serialize(serial_pose2D_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_pose2D_t));
}

void mbot_motor_vel_t_deserialize(uint8_t* src, serial_mbot_motor_vel_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_motor_vel_t));
}

void mbot_motor_vel_t_serialize(serial_mbot_motor_vel_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_motor_vel_t));
}

void twist3D_t_deserialize(uint8_t* src, serial_twist3D_t* dest) {
    memcpy(dest, src, sizeof(serial_twist3D_t));
}

void twist3D_t_serialize(serial_twist3D_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_twist3D_t));
}

void mbot_imu_t_deserialize(uint8_t* src, serial_mbot_imu_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_imu_t));
}

void mbot_imu_t_serialize(serial_mbot_imu_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_imu_t));
}

void slam_status_t_deserialize(uint8_t* src, serial_slam_status_t* dest) {
    memcpy(dest, src, sizeof(serial_slam_status_t));
}

void slam_status_t_serialize(serial_slam_status_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_slam_status_t));
}

void mbot_motor_pwm_t_deserialize(uint8_t* src, serial_mbot_motor_pwm_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_motor_pwm_t));
}

void mbot_motor_pwm_t_serialize(serial_mbot_motor_pwm_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_motor_pwm_t));
}

void pose3D_t_deserialize(uint8_t* src, serial_pose3D_t* dest) {
    memcpy(dest, src, sizeof(serial_pose3D_t));
}

void pose3D_t_serialize(serial_pose3D_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_pose3D_t));
}

void timestamp_t_deserialize(uint8_t* src, serial_timestamp_t* dest) {
    memcpy(dest, src, sizeof(serial_timestamp_t));
}

void timestamp_t_serialize(serial_timestamp_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_timestamp_t));
}

void particle_t_deserialize(uint8_t* src, serial_particle_t* dest) {
    memcpy(dest, src, sizeof(serial_particle_t));
}

void particle_t_serialize(serial_particle_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_particle_t));
}

void twist2D_t_deserialize(uint8_t* src, serial_twist2D_t* dest) {
    memcpy(dest, src, sizeof(serial_twist2D_t));
}

void twist2D_t_serialize(serial_twist2D_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_twist2D_t));
}

void mbot_encoders_t_deserialize(uint8_t* src, serial_mbot_encoders_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_encoders_t));
}

void mbot_encoders_t_serialize(serial_mbot_encoders_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_encoders_t));
}

void joy_t_deserialize(uint8_t* src, serial_joy_t* dest) {
    memcpy(dest, src, sizeof(serial_joy_t));
}

void joy_t_serialize(serial_joy_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_joy_t));
}

void point3D_t_deserialize(uint8_t* src, serial_point3D_t* dest) {
    memcpy(dest, src, sizeof(serial_point3D_t));
}

void point3D_t_serialize(serial_point3D_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_point3D_t));
}

void mbot_message_received_t_deserialize(uint8_t* src, serial_mbot_message_received_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_message_received_t));
}

void mbot_message_received_t_serialize(serial_mbot_message_received_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_message_received_t));
}
void mbot_slam_reset_t_deserialize(uint8_t* src, serial_mbot_slam_reset_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_slam_reset_t));
}

void mbot_slam_reset_t_serialize(serial_mbot_slam_reset_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_slam_reset_t));
}

void mbot_error_t_deserialize(uint8_t* src, serial_mbot_error_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_error_t));
}

void mbot_error_t_serialize(serial_mbot_error_t* src, uint8_t* dest) {
    memcpy(dest, src, sizeof(serial_mbot_error_t));
}

void encode_rospkt(uint8_t* data, uint16_t len, uint16_t topic, uint8_t* pkt) {
    // CREATE ROS PACKET
    //for ROS protocol and packet format see link: http://wiki.ros.org/rosserial/Overview/Protocol
    pkt[0] = SYNC_FLAG;
    pkt[1] = VERSION_FLAG;
    pkt[2] = (uint8_t) (len & 0xFF); //message length lower 8/16b via bitwise AND
    pkt[3] = (uint8_t) (len >> 8); //message length higher 8/16b via bitshift and cast

    uint8_t cs1_addends[2] = {pkt[2], pkt[3]};
    pkt[4] = checksum(cs1_addends, 2); //checksum over message length
    pkt[5] = (uint8_t) (topic & 0xFF); //message topic lower 8/16b via modulus and cast
    pkt[6] = (uint8_t) (topic >> 8); //message length higher 8/16b via bitshift and cast

    // for (int i = 0; i<len; i++) { //write message bytes
    //     pkt[i+7] = data[i];
    // }
    memcpy(pkt+7, data, len);
    
    uint8_t cs2_addends[len+2]; //create array for the checksum over topic and message content
    cs2_addends[0] = pkt[5];
    cs2_addends[1] = pkt[6];
    // for (int i = 0; i<len; i++) {
    //     cs2_addends[i+2] = data[i];
    // }
    memcpy(cs2_addends+2, data, len);

    pkt[len+ROS_PKG_LEN-1] = checksum(cs2_addends, len+2); //checksum over message data and topic
}

int decode_rospkt(uint8_t* pkt, uint8_t* data, uint16_t* len, uint16_t* topic) {
    // ROS PROTOCOL CHECKS
    //for ROS protocol and packet format see link: http://wiki.ros.org/rosserial/Overview/Protocol
    if (pkt[0] != SYNC_FLAG) {
        // fprintf(stderr, "Error: SYNC flag does not lead message.\n");
        ESP_LOGE("SERIALIZER", "Error: SYNC flag does not lead message.");
        return -1;
    }

    if (pkt[1] != VERSION_FLAG) {
        // fprintf(stderr, "Error: Version flag is incompatible.\n");
        ESP_LOGE("SERIALIZER", "Error: Version flag is incompatible.");
        return -1;
    }

    uint16_t msg_len = pkt[2]+(pkt[3]*255); //reconstruct message length from bytes

    uint8_t cs1_addends[2] = {pkt[2], pkt[3]}; //create array of the message length bytes
    if (pkt[4] != checksum(cs1_addends, 2)) {
        // fprintf(stderr, "Error: Checksum over message length failed.\n");
        ESP_LOGE("SERIALIZER", "Error: Checksum over message length failed.");
        return -1;
    }

    uint8_t cs2_addends[msg_len+2]; //create array for topic and message content bytes
    cs2_addends[0] = pkt[5];
    cs2_addends[1] = pkt[6];
    // for (int i = 0; i<msg_len; i++) {
    //     cs2_addends[i+2] = pkt[i+7];
    // }
    memcpy(cs2_addends+2, pkt+7, msg_len);
    if (pkt[msg_len + ROS_HEADER_LEN] != checksum(cs2_addends, msg_len+2)) {
        // fprintf(stderr, "Error: Checksum over message topic and content failed.\n");
        ESP_LOGE("SERIALIZER", "Error: Checksum over message topic and content failed.");
        return -1;
    }


    // write to out data
    // for (int i=0;i<msg_len;i++) {
    //     data[i] = pkt[i+7];
    // }
    memcpy(data, pkt+7, msg_len);
    *topic = (uint16_t) (pkt[5]+(pkt[6]*255));
    
    return 0;
}

void encode_botpkt(packets_wrapper_t* data, uint8_t* mac, uint8_t* pkt) {
    // TODO: Implement
}

int decode_botpkt(uint8_t* pkt, packets_wrapper_t* data, uint8_t* mac) {
    // TODO: Implement
    return 0;
}

void encode_cmdpkt(uint8_t* rospkt, uint16_t len, uint8_t* mac, uint8_t* cmdpkt) {
    // TODO: Implement
}

int decode_cmdpkt(uint8_t* cmdpkt, uint8_t* rospkt, uint16_t* len, uint8_t* mac) {
    // TODO: Implement
    return 0;
}