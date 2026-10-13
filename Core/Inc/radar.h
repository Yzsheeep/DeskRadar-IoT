#ifndef __RADAR_H
#define __RADAR_H

#include <stdint.h>

typedef struct{
uint8_t status;
uint16_t move_dist;
uint8_t move_energy;
uint16_t static_dist;
uint8_t static_energy;
uint16_t detect_dist;
}LD2410B_Data_t;

extern LD2410B_Data_t g_radar_data;
extern uint8_t frame_updated;

extern volatile uint32_t g_radar_rx_cnt;
extern volatile uint32_t g_radar_frame_cnt;

void Parse_Radar_Data(uint8_t *buf,uint16_t len);
uint8_t Radar_Init(void);

#endif
