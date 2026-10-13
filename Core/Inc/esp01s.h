#ifndef __ESP01S_H
#define __ESP01S_H

#include "main.h"
#include <stdint.h>

extern volatile uint8_t g_cloud_online; // 云端在线标志位，1在线，0离线

void ESP01S_Init_Wifi(void);

void ESP01S_Post_Data(uint16_t dist,uint8_t state,uint8_t alarm);


#endif