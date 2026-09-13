#include "filter.h"
#include "radar.h"  // 因为要用到 target_status 和 moving_dist，所以必须引入雷达菜单
#include "stdlib.h"

// 真正的变量定义
uint8_t display_state = 0;
uint16_t filtered_dist = 0;

void Radar_Data_Filter(void) {
    static uint32_t last_move_time = 0; 
    uint16_t current_raw = (moving_dist > 0) ? moving_dist : static_dist;

    // 1. 状态判定 (只抓核心变化)
    if (target_status == 0x01 || target_status == 0x03) {
        // 如果没人，或者距离突变超过 3cm，刷新计时器并判定为运动
        if (filtered_dist == 0 || abs(current_raw - filtered_dist) > 3) {
            last_move_time = HAL_GetTick(); 
            display_state = 1; 
        }
    }
    
    // 统一处理超时：距离上次有效运动超过 400ms
    if (HAL_GetTick() - last_move_time > 400) {
        display_state = (target_status == 0x00) ? 0 : 2; 
    }

    // 2. 距离赋值 (剥离出来单独算，逻辑极其清晰)
    if (display_state == 0) {
        filtered_dist = 0; 
    } else {
        if (filtered_dist == 0) {
            filtered_dist = current_raw; // 刚进来，直接给真实值
        } else {
            filtered_dist = (filtered_dist * 9 + current_raw) / 10; // 缓和滤波
        }
    }
}