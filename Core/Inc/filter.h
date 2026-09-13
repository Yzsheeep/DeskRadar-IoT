#ifndef __FILTER_H
#define __FILTER_H

#include "main.h"

// 暴露出最终算好的平滑数据，给屏幕显示用
extern uint8_t display_state;
extern uint16_t filtered_dist;

// 暴露滤波函数
void Radar_Data_Filter(void);

#endif