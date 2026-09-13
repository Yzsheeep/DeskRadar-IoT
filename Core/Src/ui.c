#include "ui.h"
#include "filter.h" // 因为要用到滤波后的 display_state 和 filtered_dist
#include "oled.h"   // 替换成你实际的 OLED 头文件！
#include <stdio.h>  // 为了使用 sprintf

void Update_UI(void) {
    char buf[32]; 
    OLED_NewFrame(); 
    OLED_PrintString(0, 0, "- Desk Radar -", &font16x16, OLED_COLOR_NORMAL);
    
    if (display_state == 0) { // 无人状态
        OLED_PrintString(0, 16, "Status: Nobody ", &font16x16, OLED_COLOR_NORMAL);
        OLED_PrintString(0, 32, "               ", &font16x16, OLED_COLOR_NORMAL); 
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); 
    } else {                  // 有人状态 (合并了 Moving 和 Static)
     
        sprintf(buf, "Status: %s ", (display_state == 1) ? "Moving" : "Static");
        OLED_PrintString(0, 16, buf, &font16x16, OLED_COLOR_NORMAL);
        
        sprintf(buf, "Dist: %-3d cm   ", filtered_dist); 
        OLED_PrintString(0, 32, buf, &font16x16, OLED_COLOR_NORMAL);
        
        HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);   
    }
    OLED_ShowFrame(); 
}