#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef LCM_TYPES_H
#define LCM_TYPES_H

enum message_topics {
    MBOT_TIMESYNC = 201,
    MBOT_ODOMETRY = 210,
    MBOT_ODOMETRY_RESET = 211,
    MBOT_VEL_CMD = 214,
    MBOT_IMU = 220,
    MBOT_ENCODERS = 221,
    MBOT_ENCODERS_RESET = 222,
    MBOT_MOTOR_PWM_CMD = 230,
    MBOT_MOTOR_VEL_CMD = 231,
    MBOT_MOTOR_VEL = 232,
    MBOT_MOTOR_PWM = 233,
    MBOT_VEL = 234
};

typedef struct __attribute__((__packed__)) serial_pose2D_t {
    int64_t utime;
    float x;
    float y;
    float theta;
} serial_pose2D_t;

typedef struct __attribute__((__packed__)) serial_mbot_motor_vel_t {
    int64_t utime;
    float velocity[3]; // [rad/s]
} serial_mbot_motor_vel_t;

typedef struct __attribute__((__packed__)) serial_twist3D_t {
    int64_t utime;
    float vx;
    float vy;
    float vz;
    float wx;
    float wy;
    float wz;
} serial_twist3D_t;

typedef struct __attribute__((__packed__)) serial_mbot_imu_t {
    int64_t utime;
    float gyro[3];
    float accel[3];
    float mag[3];
    float angles_rpy[3]; // roll (x), pitch (y), yaw, (z)
    float angles_quat[4]; // quaternion
    float temp;
} serial_mbot_imu_t;

typedef struct __attribute__((__packed__)) serial_slam_status_t {
    int64_t utime;
    int32_t slam_mode; // mapping_only=0, action_only=1, localization_only=2, full_slam=3
    char map_path[256]; // Path to where the map is stored.
} serial_slam_status_t;

typedef struct __attribute__((__packed__)) serial_mbot_motor_pwm_t {
    int64_t utime;
    float pwm[3]; // [-1.0..1.0]
} serial_mbot_motor_pwm_t;

typedef struct __attribute__((__packed__)) serial_pose3D_t {
    int64_t utime;
    float x;
    float y;
    float z;
    float angles_rpy[3];
    float angles_quat[4];
} serial_pose3D_t;

typedef struct __attribute__((__packed__)) serial_timestamp_t {
    int64_t utime;
} serial_timestamp_t;

typedef struct __attribute__((__packed__)) serial_particle_t {
    serial_pose2D_t pose; // (x,y,theta) pose estimate
    serial_pose2D_t parent_pose; // (x,y,theta) of the prior pose the new estimate came from
    double weight; // normalized weight of the particle as computed by the sensor model
} serial_particle_t;

typedef struct __attribute__((__packed__)) serial_twist2D_t {
    int64_t utime;
    float vx;
    float vy;
    float wz;
} serial_twist2D_t;

typedef struct __attribute__((__packed__)) serial_mbot_encoders_t {
    int64_t utime;
    int64_t ticks[3]; // no units
    int32_t delta_ticks[3]; // no units
    int32_t delta_time; // [usec]
} serial_mbot_encoders_t;

typedef struct __attribute__((__packed__)) serial_joy_t {
    int64_t timestamp;
    float left_analog_X;
    float left_analog_Y;
    float right_analog_X;
    float right_analog_Y;
    float right_trigger;
    float left_trigger;
    float dpad_X;
    float dpad_Y;
    int8_t button_A;
    int8_t button_B;
    int8_t button_2; // not used
    int8_t button_X;
    int8_t button_Y;
    int8_t button_5; // not used
    int8_t button_l1;
    int8_t button_r1;
    int8_t button_l2;
    int8_t button_r2;
    int8_t button_select;
    int8_t button_start;
    int8_t button_12; // not used
    int8_t button_left_analog;
    int8_t button_right_analog;
    int8_t button_15; //not used
} serial_joy_t;

typedef struct __attribute__((__packed__)) serial_point3D_t {
    int64_t utime;
    float x;
    float y;
    float z;
} serial_point3D_t;

typedef struct __attribute__((__packed__)) serial_mbot_message_received_t {
    int64_t utime; // Time of confirmation message creation
    int64_t creation_time; // time of message creation (assumption that this will be unique between messages)
    char channel[256]; //name of channel 
} serial_mbot_message_received_t;

typedef struct __attribute__((__packed__)) serial_mbot_slam_reset_t {
    int64_t utime;
    int32_t slam_mode; // mapping_only=0, action_only=1, localization_only=2, full_slam=3
    char slam_map_location[256]; // only necessary when for localization-only and action_only modes
    bool retain_pose; // Whether to keep the pose when resetting.
} serial_mbot_slam_reset_t;

void pose2D_t_deserialize(uint8_t* src, serial_pose2D_t* dest);
void pose2D_t_serialize(serial_pose2D_t* src, uint8_t* dest);
void mbot_motor_vel_t_deserialize(uint8_t* src, serial_mbot_motor_vel_t* dest);
void mbot_motor_vel_t_serialize(serial_mbot_motor_vel_t* src, uint8_t* dest);
void twist3D_t_deserialize(uint8_t* src, serial_twist3D_t* dest);
void twist3D_t_serialize(serial_twist3D_t* src, uint8_t* dest);
void mbot_imu_t_deserialize(uint8_t* src, serial_mbot_imu_t* dest);
void mbot_imu_t_serialize(serial_mbot_imu_t* src, uint8_t* dest);
void slam_status_t_deserialize(uint8_t* src, serial_slam_status_t* dest);
void slam_status_t_serialize(serial_slam_status_t* src, uint8_t* dest);
void mbot_motor_pwm_t_deserialize(uint8_t* src, serial_mbot_motor_pwm_t* dest);
void mbot_motor_pwm_t_serialize(serial_mbot_motor_pwm_t* src, uint8_t* dest);
void pose3D_t_deserialize(uint8_t* src, serial_pose3D_t* dest);
void pose3D_t_serialize(serial_pose3D_t* src, uint8_t* dest);
void timestamp_t_deserialize(uint8_t* src, serial_timestamp_t* dest);
void timestamp_t_serialize(serial_timestamp_t* src, uint8_t* dest);
void particle_t_deserialize(uint8_t* src, serial_particle_t* dest);
void particle_t_serialize(serial_particle_t* src, uint8_t* dest);
void twist2D_t_deserialize(uint8_t* src, serial_twist2D_t* dest);
void twist2D_t_serialize(serial_twist2D_t* src, uint8_t* dest);
void mbot_encoders_t_deserialize(uint8_t* src, serial_mbot_encoders_t* dest);
void mbot_encoders_t_serialize(serial_mbot_encoders_t* src, uint8_t* dest);
void joy_t_deserialize(uint8_t* src, serial_joy_t* dest);
void joy_t_serialize(serial_joy_t* src, uint8_t* dest);
void point3D_t_deserialize(uint8_t* src, serial_point3D_t* dest);
void point3D_t_serialize(serial_point3D_t* src, uint8_t* dest);
void mbot_message_received_t_deserialize(uint8_t* src, serial_mbot_message_received_t* dest);
void mbot_message_received_t_serialize(serial_mbot_message_received_t* src, uint8_t* dest);
void mbot_slam_reset_t_deserialize(uint8_t* src, serial_mbot_slam_reset_t* dest);
void mbot_slam_reset_t_serialize(serial_mbot_slam_reset_t* src, uint8_t* dest);

#endif