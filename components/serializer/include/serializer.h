#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "lcm_types.h"

#ifndef SERIALIZER_H
#define SERIALIZER_H


#define SYNC_FLAG       0xff
#define VERSION_FLAG    0xfe
#define ROS_HEADER_LEN  7
#define ROS_FOOTER_LEN  1
#define ROS_PKG_LEN     (ROS_HEADER_LEN + ROS_FOOTER_LEN)

typedef struct __attribute__((__packed__)) packets_wrapper {
    serial_mbot_encoders_t encoders;
    serial_pose2D_t odom;
    serial_mbot_imu_t imu;
    serial_twist2D_t mbot_vel;
    serial_mbot_motor_vel_t motor_vel;
    serial_mbot_motor_pwm_t motor_pwm;
} packets_wrapper_t;

uint8_t checksum(uint8_t* addends, int len);
void encode_rospkt(uint8_t* data, uint16_t len, uint16_t topic, uint8_t* pkt);
int decode_rospkt(uint8_t* pkt, uint8_t* data, uint16_t* len, uint16_t* topic);
void encode_botpkt(packets_wrapper_t* data, uint8_t* mac, uint8_t* pkt);
int decode_botpkt(uint8_t* pkt, packets_wrapper_t* data, uint8_t* mac);
void encode_cmdpkt(uint8_t* rospkt, uint16_t len, uint8_t* mac, uint8_t* cmdpkt);
int decode_cmdpkt(uint8_t* cmdpkt, uint8_t* rospkt, uint16_t* len, uint8_t* mac);

#endif