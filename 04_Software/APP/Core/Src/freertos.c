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

#include "queue.h" 	//队列
#include <string.h>  // memset
#include <stdint.h>//uint8_t uint32_t
#include <stdio.h>//null
#include <SWC_OTA.h>//OTA s multi Taasks

// #include "task.h"   // 任务通知函数  xTaskNotifyFromISR  MAX_DELAY
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

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
extern osThreadId_t OTATaskHandle;///协议接收Task
extern const osThreadAttr_t OTATask_attributes;

QueueHandle_t Q_YmodemReclength;

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  Q_YmodemReclength = xQueueCreate(1, sizeof(uint16_t));/// 消息队列
  
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);
  
  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  OTATaskHandle = osThreadNew(ota_task_runnable, NULL, &OTATask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
uint8_t recv_buf[10]={0};//接收命令的缓冲区

uint8_t recv_len=0;//中断回调中进行修改
/**
  * @brief  Function implementing the defaultTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, recv_buf, 10);//启动dma 接收 
   /* Infinite loop */
  for(;;)
  {
	  HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    osDelay(500);
    // for(int i=0;i<recv_len;i++)
    // {
    //   HAL_UART_Transmit(&huart1, &recv_buf[i], 1, 10);
    //   // HAL_UART_Transmit_IT(&huart1, &recv_buf[i], 1);//non block
    // }
    //   recv_len = 0;         
    // if(recv_len!=0){

    // }
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart,uint16_t Size)
{
    UNUSED(huart);
  UNUSED(Size);
  BaseType_t  xHigherPriorityTaskWoken = pdFALSE;
    if (huart->Instance == USART1)
  {
      //获取剩余当前传输值
			if(1 == __HAL_UART_GET_FLAG(huart,UART_FLAG_IDLE))
			{
				recv_len = Size;  //更新接收长度
        if(pdFALSE != xQueueSendFromISR(Q_YmodemReclength,&recv_len,0))
				{
					xHigherPriorityTaskWoken = pdTRUE;
				}
				HAL_UART_DMAStop(huart);//停止dma 数据接收完成
			}
			portYIELD_FROM_ISR(xHigherPriorityTaskWoken);//更高优先级的任务被唤醒，切换到该任务
      // DMA 传输完成后的操作

      // HAL_UARTEx_ReceiveToIdle_DMA(&huart1, recv_buf, 10);        ///在这里开启
    }

}
/* USER CODE END Application */

