#include "SWC_OTA.h"
#include "ymodem.h"


#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"
/**
 * @brief  OTA 协议接收任务
 * @param  
 * @retval None
 */
osThreadId_t OTATaskHandle;
const osThreadAttr_t OTATask_attributes = {
  .name = "OTATask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
uint8_t g_au8_YmodemRec[1030];//1024数据+3头+2校验  +1
//ymodem 协议 接收数据缓冲区

void ota_task_runnable(void *argument)
{
  while (1)
  {
    Ymodem_Receive(g_au8_YmodemRec);
  }
}