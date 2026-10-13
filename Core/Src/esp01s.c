#include "esp01s.h"
#include "usart.h"
#include "cmsis_os2.h"
#include <string.h>
#include <stdio.h>

extern UART_HandleTypeDef huart1;

volatile uint8_t g_cloud_online = 0; 

static void ESP01S_SendCmd(const char *cmd)
{
    HAL_UART_Transmit(&huart1,(uint8_t *)cmd,strlen(cmd),1000);
}

static uint8_t ESP01S_WaitForChar(char target, uint32_t timeout_ms)
{
    uint8_t ch = 0;
    uint32_t start = osKernelGetTickCount();
    while ((osKernelGetTickCount() - start) < timeout_ms)
    {
        if (HAL_UART_Receive(&huart1, &ch, 1, 10) == HAL_OK)
        {
            if (ch == target)
            {
                return 1; // 成功匹配目标字符
            }
        }
    }
    return 0; // 超时
}

void ESP01S_Init_Wifi(void)
{
    g_cloud_online = 0;
    
    osDelay(1000);

    ESP01S_SendCmd("AT+RST\r\n");
    osDelay(2000);

    ESP01S_SendCmd("AT\r\n");
    osDelay(500);

    ESP01S_SendCmd("AT+CWMODE=1\r\n");
    osDelay(500);

    ESP01S_SendCmd("AT+CWJAP=\"Xiaomi15Pro\",\"Zhaoyang@123\"\r\n");
    osDelay(6000);

    ESP01S_SendCmd("AT+MQTTUSERCFG=0,1,\"Test1\",\"hs031zf1EK\",\"version=2018-10-31&res=products%2Fhs031zf1EK%2Fdevices%2FTest1&et=1819019040&method=md5&sign=GTh3jo7RH51nqGuNWJKK%2Fg%3D%3D\",0,0,\"\"\r\n");
    osDelay(1000);

    ESP01S_SendCmd("AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n");
    osDelay(2000);

    g_cloud_online = 1;    

}

void ESP01S_Post_Data(uint16_t dist,uint8_t state,uint8_t alarm)
{
   if(!g_cloud_online)
    {
        return;
    }
    char payload[256]; // 改为 256，彻底避免内存截断溢出！
    char cmd[160];

    int len = sprintf(payload,
         "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"distance\":{\"value\":%d},\"status\":{\"value\":%d},\"alarm\":{\"value\":%d}}}",
         dist, state, alarm);
    
    // 清空串口接收寄存器中的陈旧数据，防止干扰判定
    __HAL_UART_FLUSH_DRREGISTER(&huart1);

    sprintf(cmd, "AT+MQTTPUBRAW=0,\"$sys/hs031zf1EK/Test1/thing/property/post\",%d,0,0\r\n", len);
    ESP01S_SendCmd(cmd);
    if (ESP01S_WaitForChar('>', 400))
    {
        // 收到 '>' 后立即发送真实的 payload 报文
        HAL_UART_Transmit(&huart1, (uint8_t *)payload, len, 500);
    }
    else
    {
        // 如果 400ms 没等到 '>'，放弃本次发送，不要把 payload 喷进去污染串口！
        // 这一步彻底阻断了状态机死锁
    }
}

