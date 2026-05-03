/*
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file manage_jmp.c
 * 
 * @par dependencies 
 * - stdio.h
 * - stdint.h
 * - "usart.h"
 * - elog.h

 * @author yan | R&D Dept. | EternalChip ?????
 *
 * @brief Provides HAL APIs for LED control and operations.
 * 
 * Usage:
 * Call functions directly.
 * 
 * @version V1.0 2026年4月24日
 *
 * @note 1 tab == 4 spaces
 * 
 *****************************************************************************/

/* Includes ------------------------------------------------------------------*/
#include "manage_jmp.h"

#define FALSH_BASE_ADDR  ((uint32_t)0x08000000)//+ 0x00019000
// #define FALSH_BASE_ADDR 0x08000000//+ 0x00019000
#define ApplicationAddress 0x08008000 //APP程序的启动地址-MSP
typedef void (*pFunc)(void);      //pFunc 是变量名'，类型是 void (*)(void)。
pFunc Jump2Application;//函数指针类型--变量 
                        //完全等价与 void (*Jump2Application)(void) 
uint32_t JumpAddress;
void Jump2App(void){
    /* 检查栈顶地址是否合法 */
    if(((*(__IO uint32_t *)ApplicationAddress) & 0x2FFE0000) == 0x20000000)
    {

        /* 屏蔽所有中断，防止在跳转过程中，中断干扰出现异常 */
        __disable_irq();
        NVIC_SetVectorTable(FALSH_BASE_ADDR, 0x8000);
        RCC_DeInit();
        /* 用户代码区第二个 字 为程序开始地址(复位地址) */
        JumpAddress = *(__IO uint32_t *) (ApplicationAddress + 4);

        /* Initialize user application's Stack Pointer */
        /* 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址) */
        __set_MSP(*(__IO uint32_t *) ApplicationAddress);

        /* 类型转换 */
        Jump2Application = (pFunc) JumpAddress;

        /* 跳转到 APP */
        Jump2Application();
    }
  }
#if 0
        // NVIC_SetVectorTable(FALSH_BASE_ADDR, 0x8000);
//        NVIC_SetVectorTable(FALSH_BASE_ADDR, 0x0000);
//		    // 2. 关闭SysTick（关键！）
//    SysTick->CTRL = 0;
//    SysTick->LOAD = 0;
//    SysTick->VAL = 0;
  
  /**关闭外设函数 */
void DisablePeriphClock_irq(void){
/**  //串口关闭
  // HAL_UART_DeInit(&huart1);
  //关闭所有外设时钟/
  __HAL_RCC_RTC_DISABLE();
  ///中断禁用
  __disable_irq(); */
    // HAL_DeInit();
    // HAL_RCC_DeInit();

    RCC_DeInit();
    // 关闭所有中断---防止加了rtos出问题
    __set_PRIMASK(1);
      // 2. 彻底关闭 SysTick 并且清除所有NVIC中断（极其关键！）
    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL = 0;
    for (int i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFF; // 关闭全部中断
        NVIC->ICPR[i] = 0xFFFFFFFF; // 清除所有挂起的中断
    }
}
/**跳转函数 */
void Jump2App(void){
//  uint32_t jumpAddr,,i;
// 	uint32_t armAddr;
	
// 	armAddr = *(__IO uint32_t*)APP_FLASH_ADDR; //读取app程序的初始堆栈地址
// 	printf("app stack :0x%08X\r\n",armAddr);
// uint32_t mspAddr;mspAddr = *(__IO uint32_t*)FALSH_BASE_ADDR;
// 	printf("msp :0x%08X\r\n",mspAddr);

  if(((*(__IO uint32_t*)APP_FLASH_ADDR) & 0x2FFE0000) == 0x20000000){ //判断地址是否合法//0x2FFE0000
    printf("jump addr start\r\n");  
 	DisablePeriphClock_irq(); 
      /**取出app加载地址0x00019000-》将数字转成指针（地址）-》
       * 将falsh中的地址解引用查到地址中的存好的值-》判断这个值是否是RAM地址-128kb
       * */
    // 1. 从Flash读取应用程序的复位向量地址（PC指针）----这里是ram地址????
    JumpAddress = *(__IO uint32_t*)(APP_FLASH_ADDR + 4);
//    printf("jump addr :0x%08X\r\n",JumpAddress);

  
    // 2. 把地址转成函数指针类型，赋值给变量
    Jump2Application = (pFunc)JumpAddress;//类型是 void (*)(void)
    // 3. 设置主堆栈指针（MSP）为应用程序的初始堆栈地址
    __set_MSP(*(__IO uint32_t*)APP_FLASH_ADDR);/////////将栈顶指针手动设置
    // 4. 跳转到应用程序的复位向量地址，开始执行应用程序
    Jump2Application();

  }else{
    printf("jump app addr error\r\n");
  }
}
  #endif
