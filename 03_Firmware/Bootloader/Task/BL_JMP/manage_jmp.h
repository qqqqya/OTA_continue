#ifndef __MANAGE_JMP_H__
#define __MANAGE_JMP_H__

#include "main.h"
#include "aes.h"
#include "w25qxx_Handler.h"

#define FALSH_BASE_ADDR  ((uint32_t)0x08000000)//+ 0x00019000
// #define FALSH_BASE_ADDR 0x08000000//+ 0x00019000
#define ApplicationAddress 0x08008000 //APP程序的启动地址-MSP
#define BackupApplicationAddress 0x08020000 //备份APP程序启动地址

#define NO_APP_UPDATE               0x00   // 无App更新（正常运行状态）
#define APP_DOWNLOADING             0x11   // App正在下载中
#define APP_DOWNLOAD_COMPLETE       0x22   // App下载完成
#define APP_FIRST_CHECK_START       0x33   // ★ App首次校验启动（看门狗检测入口）
#define APP_FIRST_CHECKING          0x44   // ★ App首次校验中（看门狗复位后到达的状态）

void Jump2App(void);
int8_t Backup2App(void);
int8_t AES_Backup2App(int32_t fl_size);
int8_t External_AES_Backup2App(int32_t fl_size);

void Ota_statemanage(void);
int8_t ExA_To_ExB_AES(int32_t fl_size);
int8_t ExB_To_App(void);
int8_t ExA_To_App(void);
int8_t App_To_ExA(int32_t fl_size);
#endif 
