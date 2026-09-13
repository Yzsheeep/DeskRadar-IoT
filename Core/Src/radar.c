#include "radar.h"

volatile uint32_t rx_count = 0; // 用来记录到底收到了多少个字节
/* ================= 雷达解析专用变量 ================= */
uint8_t rx_byte;          // 用来装每次收到的 1 个字节
uint8_t radar_buffer[20]; // 缓冲区，用来存一帧的数据
uint8_t rx_index = 0;     // 缓冲区索引
uint8_t parse_state = 0;  // 状态机状态
uint8_t frame_updated = 0;

// 这是我们要提取出的核心数据！
uint8_t target_status = 0; // 目标状态：0无目标，1运动，2静止，3运动&静止
uint16_t moving_dist = 0;  // 运动目标距离
uint16_t static_dist = 0;  // 静止目标距离
char display_buf[32];      // 用来给 OLED 显示的字符串

/* ================= 核心解析函数 ================= */
// 把串口收到的每一个字节，丢进这个函数里
void Parse_Radar_Data(uint8_t data) {
    switch(parse_state) {
        // 第一步：寻找 4 个字节的帧头 F4 F3 F2 F1
        case 0: if(data == 0xF4) parse_state = 1; else parse_state = 0; break;
        case 1: if(data == 0xF3) parse_state = 2; else parse_state = 0; break;
        case 2: if(data == 0xF2) parse_state = 3; else parse_state = 0; break;
        case 3: 
            if(data == 0xF1) { 
                parse_state = 4;  // 帧头完全对上！进入接收数据阶段
                rx_index = 0;     // 索引清零
            } else {
                parse_state = 0;
            }
            break;
            
        // 第二步：接收帧内数据
        case 4:
            radar_buffer[rx_index++] = data;
            
            // 根据协议，正常工作模式下帧内数据加上校验大概是 15 个字节
            if(rx_index >= 15) { 
                // 数据接收完毕，开始提取有用信息！
                // 协议规定：数据类型为 0x02（目标基本信息），帧内头部为 0xAA
                if ((radar_buffer[2] == 0x02 || radar_buffer[2] == 0x01) && radar_buffer[3] == 0xAA) {
                    
                    target_status = radar_buffer[4]; // 提取目标状态
                    
                    // 提取距离（串口通信是小端格式，低位在前，高位在后）
                    moving_dist = radar_buffer[5] | (radar_buffer[6] << 8); // 运动距离
                    static_dist = radar_buffer[8] | (radar_buffer[9] << 8); // 静止距离

                    frame_updated = 1;
                }
                
                parse_state = 0; // 这一帧解析完了，状态清零，准备抓取下一帧
            }
            break;
    }
}