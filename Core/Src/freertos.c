/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include "queue.h"
#include "oled.h"
#include "radar.h"
#include "esp01s.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
typedef enum {
    SEC_SAFE = 0,    // 安全：无入侵
    SEC_SUSPECT,     // 疑似：雷达检测到目标，进入时间窗口确认
    SEC_ALERT        // 报警：确认入侵！
} Security_Status_t;

typedef struct {
    uint8_t raw_state;       // 雷达原始状态 (0/1/2/3)
    uint16_t dist;           // 目标距离 (cm)
    Security_Status_t sec_level; // 安防定性状态
} RadarData_t;

// 用于通知 CloudTask 立即上报告警的事件标志
volatile uint8_t g_need_instant_post = 0;

/* USER CODE END Variables */
/* Definitions for RadarTask */
osThreadId_t RadarTaskHandle;
const osThreadAttr_t RadarTask_attributes = {
  .name = "RadarTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for DisplayTask */
osThreadId_t DisplayTaskHandle;
const osThreadAttr_t DisplayTask_attributes = {
  .name = "DisplayTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* Definitions for CloudTask */
osThreadId_t CloudTaskHandle;
const osThreadAttr_t CloudTask_attributes = {
  .name = "CloudTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};
/* Definitions for radarQueue */
osMessageQueueId_t radarQueueHandle;
const osMessageQueueAttr_t radarQueue_attributes = {
  .name = "radarQueue"
};
/* Definitions for uartMutex */
osMutexId_t uartMutexHandle;
const osMutexAttr_t uartMutex_attributes = {
  .name = "uartMutex"
};
/* Definitions for testSem */
osSemaphoreId_t testSemHandle;
const osSemaphoreAttr_t testSem_attributes = {
  .name = "testSem"
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

void StartRadarTask(void *argument);
void StartDisplayTask(void *argument);
void StartCloudTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */
  /* Create the mutex(es) */
  /* creation of uartMutex */
  uartMutexHandle = osMutexNew(&uartMutex_attributes);

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* Create the semaphores(s) */
  /* creation of testSem */
  testSemHandle = osSemaphoreNew(1, 1, &testSem_attributes);

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of radarQueue */
  radarQueueHandle = osMessageQueueNew (16, sizeof(RadarData_t), &radarQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of RadarTask */
  RadarTaskHandle = osThreadNew(StartRadarTask, NULL, &RadarTask_attributes);

  /* creation of DisplayTask */
  DisplayTaskHandle = osThreadNew(StartDisplayTask, NULL, &DisplayTask_attributes);

  /* creation of CloudTask */
  CloudTaskHandle = osThreadNew(StartCloudTask, NULL, &CloudTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartRadarTask */
/**
  * @brief  Function implementing the RadarTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartRadarTask */
void StartRadarTask(void *argument)
{
  /* USER CODE BEGIN StartRadarTask */
  RadarData_t send_data = {0};
  uint8_t hit_count = 0; // 连续检出帧计数器
  Security_Status_t last_level = SEC_SAFE;
  Radar_Init();

  /* Infinite loop */
  for(;;)
  {

    if(frame_updated == 1)
    {
      frame_updated = 0;

      send_data.raw_state = g_radar_data.status;
      send_data.dist = g_radar_data.detect_dist;

      if(send_data.raw_state > 0)
      {
        if(hit_count < 15) hit_count++;
      }
      else
      {
        if(hit_count > 0) hit_count--;
      }
      if(hit_count >= 5)
      {
        send_data.sec_level = SEC_ALERT;
                // 首次切入报警状态时，立即触发云端紧急上报
        if(last_level != SEC_ALERT)
          {
            g_need_instant_post = 1;
          }
      }
        else if(hit_count > 0)
        {
          send_data.sec_level = SEC_SUSPECT;
        }
        else
        {
          send_data.sec_level = SEC_SAFE;
        }

      last_level = send_data.sec_level;

      osMessageQueuePut(radarQueueHandle, &send_data, 0, 0);
    }

    osDelay(20);

  }
  /* USER CODE END StartRadarTask */
}

/* USER CODE BEGIN Header_StartDisplayTask */
/**
* @brief Function implementing the DisplayTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartDisplayTask */
void StartDisplayTask(void *argument)
{
  /* USER CODE BEGIN StartDisplayTask */
  RadarData_t recv_data = {0};
  char disp_buf[32];
  osStatus_t status;

  OLED_NewFrame();
  OLED_PrintString(0, 0, "[ SEC SYSTEM ]", &font16x16, OLED_COLOR_NORMAL);
  OLED_PrintString(0, 16, "Arming System...", &font16x16, OLED_COLOR_NORMAL);
  OLED_ShowFrame();
  /* Infinite loop */
  for(;;)
  {
    status = osMessageQueueGet(radarQueueHandle, &recv_data, NULL, 100);
    OLED_NewFrame();
    OLED_PrintString(0,0, "[ SEC SYSTEM ]",&font16x16, OLED_COLOR_NORMAL);

    
    if(status==osOK)
    {


    if(recv_data.sec_level == SEC_SAFE)
    {
       OLED_PrintString(0, 16, "Status: SAFE", &font16x16, OLED_COLOR_NORMAL);
       OLED_PrintString(0, 32, "Dist  : --- cm", &font16x16, OLED_COLOR_NORMAL);
       HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET);

    }
    else if (recv_data.sec_level == SEC_SUSPECT)
    {
      OLED_PrintString(0, 16, "Status: LOCKING", &font16x16, OLED_COLOR_NORMAL);
      sprintf(disp_buf, "Dist  : %-3d cm", recv_data.dist);
      OLED_PrintString(0, 32, disp_buf, &font16x16, OLED_COLOR_NORMAL);
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_SET); 
    }
    else if (recv_data.sec_level == SEC_ALERT)
    {
      OLED_PrintString(0, 16, "! INTRUSION !", &font16x16, OLED_COLOR_NORMAL);
      sprintf(disp_buf, "Target: %-3d cm", recv_data.dist);
      OLED_PrintString(0, 32, disp_buf, &font16x16, OLED_COLOR_NORMAL);
      // 告警时板载 LED 快速翻转闪烁
      HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET); 
    }
    }
    else
    {
      // 队列暂时没有数据时，明确提示等待数据
      OLED_PrintString(0, 16, "Sensor Paused", &font16x16, OLED_COLOR_NORMAL);
    }




    OLED_ShowFrame();

  }
 
  /* USER CODE END StartDisplayTask */
}

/* USER CODE BEGIN Header_StartCloudTask */
/**
* @brief Function implementing the CloudTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCloudTask */
void StartCloudTask(void *argument)
{
  /* USER CODE BEGIN StartCloudTask */
  ESP01S_Init_Wifi();

  /* Infinite loop */
  
    for(;;)
    {
        if(g_cloud_online)
        {
          uint16_t snap_dist = g_radar_data.detect_dist;
          uint8_t snap_status = g_radar_data.status;
          uint8_t alarm_val = (snap_status > 0) ? 1 : 0;
            
          ESP01S_Post_Data(snap_dist, snap_status, alarm_val);
        }
        
        osDelay(1000); 
    }
     
  /* USER CODE END StartCloudTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

