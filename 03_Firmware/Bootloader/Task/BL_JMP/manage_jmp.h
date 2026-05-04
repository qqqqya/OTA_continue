#ifndef __MANAGE_JMP_H__
#define __MANAGE_JMP_H__

#include "main.h"

#define FALSH_BASE_ADDR  ((uint32_t)0x08000000)//+ 0x00019000
// #define FALSH_BASE_ADDR 0x08000000//+ 0x00019000
#define ApplicationAddress 0x08008000 //APP程序的启动地址-MSP

void Jump2App(void);

#endif 
