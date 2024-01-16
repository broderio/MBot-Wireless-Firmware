#include <stdio.h>

#include "serializer.h"
#include "lcm_types.h"

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

void encode_rospkt(uint8_t* data, uint16_t len, uint16_t topic, uint8_t* pkt) {
    // TODO: Implement
}

int decode_rospkt(uint8_t* pkt, uint8_t* data, uint16_t* len, uint16_t* topic) {
    // TODO: Implement
    return 0;
}