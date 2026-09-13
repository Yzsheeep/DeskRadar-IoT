#ifndef __RADAR_H
#define __RADAR_H

#include "main.h"

extern volatile uint32_t rx_count; // 用来记录到底收到了多少个字节
/* ================= 雷达解析专用变量 ================= */
extern uint8_t rx_byte;          // 用来装每次收到的 1 个字节
extern uint8_t radar_buffer[20]; // 缓冲区，用来存一帧的数据
extern uint8_t rx_index;     // 缓冲区索引
extern uint8_t parse_state;  // 状态机状态
extern uint8_t frame_updated;

// 这是我们要提取出的核心数据！
extern uint8_t target_status; // 目标状态：0无目标，1运动，2静止，3运动&静止
extern uint16_t moving_dist;  // 运动目标距离
extern uint16_t static_dist ;  // 静止目标距离
extern char display_buf[32];      // 用来给 OLED 显示的字符串

void Parse_Radar_Data(uint8_t data);

#endif