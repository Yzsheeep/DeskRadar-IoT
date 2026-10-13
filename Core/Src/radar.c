#include "radar.h"
#include "usart.h"
#include <stdint.h>


LD2410B_Data_t g_radar_data = {0};
uint8_t frame_updated = 0;

volatile uint32_t g_radar_rx_cnt = 0;   // 收到物理字节总数
volatile uint32_t g_radar_frame_cnt = 0;// 成功解包帧总数

static uint8_t parse_state = 0;
static uint8_t radar_dma_rx_buf[128];

extern UART_HandleTypeDef huart2;
static uint8_t radar_rx_byte = 0;

void Parse_Radar_Data(uint8_t *buf,uint16_t len)
{


    for (uint16_t i = 0; (i + 23) <= len; i++)
    {
        // 1. 严格判断帧头
        if (buf[i] == 0xF4 && buf[i+1] == 0xF3 && buf[i+2] == 0xF2 && buf[i+3] == 0xF1)
        {
            uint16_t data_len = buf[i+4] | (buf[i+5] << 8);//帧内数据长度
            
            if (i + 6 + data_len + 4 > len) 
            {
                continue;  // 防越界保护
            }

            uint16_t tail_idx = i + 6 + data_len;
            if (buf[tail_idx] == 0xF8 && buf[tail_idx+1] == 0xF7 && buf[tail_idx+2] == 0xF6 && buf[tail_idx+3] == 0xF5)
            {
                // 指向帧内数据区
                uint8_t *p = &buf[i+6];

                if ((p[0] == 0x01 || p[0] == 0x02) && p[1] == 0xAA && p[data_len - 2] == 0x55 &&p[data_len - 1] == 0x00)
                {
                    
                    g_radar_data.status        = p[2];
                    g_radar_data.move_dist     = (uint16_t)(p[3]  | (p[4] << 8));
                    g_radar_data.move_energy   = p[5];
                    g_radar_data.static_dist   = (uint16_t)(p[6]  | (p[7] << 8));
                    g_radar_data.static_energy = p[8];
                    g_radar_data.detect_dist   = (uint16_t)(p[9] | (p[10] << 8));

                    frame_updated = 1;
                    g_radar_frame_cnt++;
                    
                    i = tail_idx + 3; 
                }
            }
        }
    }
}

uint8_t Radar_Init(void)
{
    __HAL_UART_CLEAR_OREFLAG(&huart2);
   HAL_StatusTypeDef ret = HAL_UARTEx_ReceiveToIdle_DMA(&huart2,radar_dma_rx_buf,sizeof(radar_dma_rx_buf));
   
   __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);      //关闭DMA的半传输中断

   return (uint8_t)ret;
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
    if(huart->Instance == USART2)
    {
        g_radar_rx_cnt++; // 只要硬件引脚收到 1 个字节，计数器立刻 +1
        Parse_Radar_Data(radar_dma_rx_buf, Size);

        __HAL_UART_CLEAR_OREFLAG(&huart2);  //清除错误标志

        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, radar_dma_rx_buf, sizeof(radar_dma_rx_buf));
        __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);      //关闭DMA的半传输中断

        
    }
}

// 防溢出错误崩溃
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if(huart->Instance == USART2)
    {
        __HAL_UART_CLEAR_OREFLAG(&huart2);
        HAL_UARTEx_ReceiveToIdle_DMA(&huart2, radar_dma_rx_buf, sizeof(radar_dma_rx_buf));
        __HAL_DMA_DISABLE_IT(huart2.hdmarx, DMA_IT_HT);      //关闭DMA的半传输中断
    }
}