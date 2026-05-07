/**
  ******************************************************************************
	WeAct 微行创新 
	>> 标准库实例例程
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "tim.h"
#include "Gpio.h"
#include "debug_log.h"
#include "elog.h"
#include "manage_jmp.h"
#include "USART.h"
#include "Flash.h"
#include "Ymodem.h"
#include "Spi.h"
#include "w25qxx_Handler.h"
// 全局定义 STM32F411xE 或者 STM32F401xx
// 当前定义 STM32F411xE

// STM32F411 外部晶振25Mhz，考虑到USB使用，内部频率设置为96Mhz
// 需要100mhz,自行修改system_stm32f4xx.c

/** @addtogroup Template_Project
  * @{
  */

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static __IO uint32_t uwTimingDelay;
RCC_ClocksTypeDef RCC_Clocks;

/* Private function prototypes -----------------------------------------------*/

/* Private functions ---------------------------------------------------------*/
 /*
  *power by WeAct Studio
  *The board with `WeAct` Logo && `version number` is our board, quality guarantee. 
  *For more information please visit: https://github.com/WeActTC/MiniF4-STM32F4x1
  *更多信息请访问：https://gitee.com/WeActTC/MiniF4-STM32F4x1
  */
/**
  * @brief  Main program
  * @param  None
  * @retval None
  */
int32_t fil_size = 0;
uint8_t au8_test[1024]; 
int main(void)
{
	/* Enable Clock Security System(CSS): this will generate an NMI exception
     when HSE clock fails *****************************************************/
  RCC_ClockSecuritySystemCmd(ENABLE);
	
 /*!< At this stage the microcontroller clock setting is already configured, 
       this is done through SystemInit() function which is called from startup
       files before to branch to application main.
       To reconfigure the default setting of SystemInit() function, 
       refer to system_stm32f4xx.c file */

  /* SysTick end of count event each 1ms */
  SystemCoreClockUpdate();
  RCC_GetClocksFreq(&RCC_Clocks);
  SysTick_Config(RCC_Clocks.HCLK_Frequency / 1000);
  /* Add your application code here */
  /* Insert 50 ms delay */
  //Delay(50);
	Key_IO_Init();
	Led_IO_Init();
  //TIM_Config();
	USART1_Init();
	debug_log_init();
  log_a("This is Bootloader!");

  SPI1_Init();
  W25Q64_Init();
	//Ymodem_Receive(au8_test);
	//JumpToApp();
  if(Key_Scan())
  {
    //按下
    /*1.下载到备份区*/
    fil_size = Ymodem_Receive(au8_test);
    /*2.备份区数据拷贝到A区中*/
    if(0 == External_AES_Backup2App(fil_size))
    {
      Jump2App();
    }
    else
    {
      //
    }
  }
  else
  {
    Jump2App();
  }
  /* Infinite loop */
  while (1)
  {
    log_e("No Valid App,Please press key and download new App!");
    if(Key_Scan())
    {
      //按下
      /*1.下载到备份区*/
      fil_size = Ymodem_Receive(au8_test);
      /*2.备份区数据拷贝到A区中*/
      if(0 == External_AES_Backup2App(fil_size))
      {
        Jump2App();
      }
      else
      {
        //
      }
    }
    Delay(50);
		// //如果是按下，则Led翻转
		// if(Key_Scan())
		// {
		// 	//log_a("LED ON");
		// 	USART_SendChar(USART1,'A');
		// 	LED_ON;
		// }
		// else
		// {
		// 	USART_SendChar(USART1,'B');
		// 	//log_a("LED OFF");
		// 	LED_OFF;
		// }
	}
}

/**
  * @brief  Inserts a delay time.
  * @param  nTime: specifies the delay time length, in milliseconds.
  * @retval None
  */
void Delay(__IO uint32_t nTime)
{ 
  uwTimingDelay = nTime;

  while(uwTimingDelay != 0);
}

/**
  * @brief  Decrements the TimingDelay variable.
  * @param  None
  * @retval None
  */
void TimingDelay_Decrement(void)
{
  if (uwTimingDelay != 0x00)
  { 
    uwTimingDelay--;
  }
}

#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{ 
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {
  }
}
#endif

/**
  * @}
  */


/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
