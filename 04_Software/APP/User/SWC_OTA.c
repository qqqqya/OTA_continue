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
extern QueueHandle_t Q_YmodemReclength;/// 接收数据长度---队列

typedef enum
{
    WaitReqDownload=0,	//等待服务器请求请求
    OtaDownload,		    //OTA数据下载中
    WaitReqUpdate,	    //等待服务器请求更新App
    OtaEnd,			        //OTA任务结束
} E_Ota_State;
static E_Ota_State g_Ota_State=WaitReqDownload;


static uint8_t s_au8_OtaCmd[4];//接收的命令--与设定的命令进行校验

uint8_t t_au8_AckCmd[] = {0x44,0x55,0x66};

/* Private function prototypes -----------------------------------------------*/
static int8_t keyscan(void);
void soft_reset(void);
/* Private functions ---------------------------------------------------------*/


void ota_task_runnable(void *argument)
{
	uint8_t t_u8_rec_length=0;//接收数据长度
  while (1)
  {
    switch(g_Ota_State){
      case WaitReqDownload:
        /*1.阻塞式接收串口命令-------接收到的是串口命令*/
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_au8_OtaCmd, 4);
        /*2.等待数据接收完成*/
        xQueueReceive(Q_YmodemReclength,&t_u8_rec_length,portMAX_DELAY);
        /*3.校验数据长度*/
        if(3 == t_u8_rec_length){
          /*4.校验数据内容---是否等于设定的命令*/
          if((0x11 == s_au8_OtaCmd[0]) && (0x22 == s_au8_OtaCmd[1]) && (0x33 == s_au8_OtaCmd[2]))
          {
            /*切换状态---到下载状态*/
            g_Ota_State = OtaDownload;
            /*创建线程以及相关队列*/
            //TODO Add
          }
          else
          {/** @brief 重置接收的命令*/
            s_au8_OtaCmd[0] = 0;
            s_au8_OtaCmd[1] = 0;
            s_au8_OtaCmd[2] = 0;
            s_au8_OtaCmd[3] = 0;
          }
        }
        else
        {/** @brief 非3字节命令，重置接收的命令*/
          s_au8_OtaCmd[0] = 0;
          s_au8_OtaCmd[1] = 0;
          s_au8_OtaCmd[2] = 0;
          s_au8_OtaCmd[3] = 0;
        }
        break;
      case OtaDownload:/** @brief OTA数据下载中 下载完成还是要等命令*/
      if(0<Ymodem_Receive(g_au8_YmodemRec)){
        g_Ota_State = WaitReqUpdate;
        /*接收数据成功  发送确认命令44 55 66*/
        HAL_UART_Transmit(&huart1, t_au8_AckCmd, 3, 0xFFFF);
      }
      else
      {/** @brief 接收数据失败*/
        g_Ota_State = WaitReqDownload;
      }
        break;

      case WaitReqUpdate:/** @brief 等待PC请求更新App*/
        HAL_UARTEx_ReceiveToIdle_DMA(&huart1, s_au8_OtaCmd, 4);
        /*2.等待数据接收完成*/
        xQueueReceive(Q_YmodemReclength,&t_u8_rec_length,portMAX_DELAY);
        /*3.校验数据长度*/
        if(3 == t_u8_rec_length){
          /*4.校验数据内容---是否等于设定的命令*/
          if((0x77 == s_au8_OtaCmd[0]) && (0x88 == s_au8_OtaCmd[1]) && (0x99 == s_au8_OtaCmd[2]))
          {
            /*切换状态---到结束状态*/
            g_Ota_State = OtaEnd;
            /*创建线程以及相关队列*/
            //TODO Add
          }
          else
          {/** @brief 重置接收的命令*/
            s_au8_OtaCmd[0] = 0;
            s_au8_OtaCmd[1] = 0;
            s_au8_OtaCmd[2] = 0;
            s_au8_OtaCmd[3] = 0;
          }
        }
        else
        {/** @brief 非3字节命令，重置接收的命令*/
          s_au8_OtaCmd[0] = 0;
          s_au8_OtaCmd[1] = 0;
          s_au8_OtaCmd[2] = 0;
          s_au8_OtaCmd[3] = 0;
        }
        break;

      case OtaEnd:/** @brief OTA任务结束*/
        /*判断按键，20S内是否有按下*/
        if(1==keyscan()){
          /*按键按下  重置*/
          soft_reset();
        }else{
          /*到等待下载状态   死等按键按下*/
            g_Ota_State = WaitReqDownload;
        }
        break;
      default:/** @brief 无动作*/
        break;
    }
    // Ymodem_Receive(g_au8_YmodemRec);
  }
}
static int8_t keyscan(void){
  for(uint16_t i=0;i<400;i++){
    osDelay(50);//400*50ms=20s  判断是否按下

    if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin)){
      osDelay(20);
      /*去抖*/
      if(GPIO_PIN_RESET==HAL_GPIO_ReadPin(KEY_GPIO_Port, KEY_Pin)){
        return 1;
      }
    }
  }
  return -1;
}

void soft_reset(void){
	__set_FAULTMASK(1);
    NVIC_SystemReset();
}
