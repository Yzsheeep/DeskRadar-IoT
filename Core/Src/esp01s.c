#include "esp01s.h"
#include "oled.h"
#include <string.h>
#include <stdio.h> 

// 必须是 huart1，这是你 PCB 上接 ESP-01S 的真实引脚
extern UART_HandleTypeDef huart1; 

void ESP01S_Init_WiFi(void)
{
    OLED_NewFrame();
    OLED_PrintString(0, 0, "WiFi Init...", &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    HAL_Delay(2000); 

    // 1. 复位与基础配置
    char cmd_rst[] = "AT+RST\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd_rst, strlen(cmd_rst), 1000);
    HAL_Delay(2000); 

    char cmd_at[] = "AT\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd_at, strlen(cmd_at), 1000);
    HAL_Delay(500); 

    char cmd_mode[] = "AT+CWMODE=1\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd_mode, strlen(cmd_mode), 1000);
    HAL_Delay(500);

    // 2. 连入手机热点 (注意去掉了空格)
    OLED_PrintString(0, 16, "Connecting AP", &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    char cmd_wifi[] = "AT+CWJAP=\"Xiaomi15Pro\",\"Zhaoyang@123\"\r\n"; 
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd_wifi, strlen(cmd_wifi), 1000);
    HAL_Delay(5000); 

    // 3. 配置 OneNet MQTT 身份
    OLED_PrintString(0, 32, "Auth OneNet...", &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    char cmd_mqtt_cfg[] = "AT+MQTTUSERCFG=0,1,\"Test1\",\"hs031zf1EK\",\"version=2018-10-31&res=products%2Fhs031zf1EK%2Fdevices%2FTest1&et=1819019040&method=md5&sign=GTh3jo7RH51nqGuNWJKK%2Fg%3D%3D\",0,0,\"\"\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd_mqtt_cfg, strlen(cmd_mqtt_cfg), 1000);
    HAL_Delay(2000);

    // 4. 连接 OneNet 服务器
    OLED_PrintString(0, 48, "Link Server...", &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    char cmd_mqtt_conn[] = "AT+MQTTCONN=0,\"mqtts.heclouds.com\",1883,1\r\n";
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd_mqtt_conn, strlen(cmd_mqtt_conn), 1000);
    HAL_Delay(3000); 
    
    OLED_NewFrame();
    OLED_PrintString(0, 0, "Cloud Online!", &font16x16, OLED_COLOR_NORMAL);
    OLED_ShowFrame();
    HAL_Delay(1000);
}

// 5. 核心上报逻辑 (改用 RAW 模式透传，完美避开 JSON 截断 Bug)
void ESP01S_Post_Data(uint16_t dist, uint8_t state) {
    char payload[128]; // 压缩一半空间
    char cmd[64];
    
    // sprintf 会自动返回拼接后的字符串长度，存进 len，下面直接用
    int len = sprintf(payload, "{\"id\":\"1\",\"version\":\"1.0\",\"params\":{\"distance\":{\"value\":%d},\"status\":{\"value\":%d}}}", dist, state);
    
    sprintf(cmd, "AT+MQTTPUBRAW=0,\"$sys/hs031zf1EK/Test1/thing/property/post\",%d,0,0\r\n", len);
    HAL_UART_Transmit(&huart1, (uint8_t*)cmd, strlen(cmd), 1000);
    
    HAL_Delay(100); 
    // 直接使用 len，不再调用 strlen，提高运行效率
    HAL_UART_Transmit(&huart1, (uint8_t*)payload, len, 1000);
}