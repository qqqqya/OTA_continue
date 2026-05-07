/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file W25Q_Handler.h
 * 
 * @par dependencies 
 * - W25Q_Handler.h
 * 
 * @author Jack | R&D Dept. | EternalChip 立芯嵌入式
 * 
 * @brief Functions related to reading and writing in the chip's flash area.
 * 
 * Processing flow:
 * 
 * call directly.
 * 
 * @version V1.0 2024-09-13
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __W25Q_HANDLER_H
#define __W25Q_HANDLER_H
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "stm32f4xx.h"

/* Exported types ------------------------------------------------------------*/
/*
最小读写单元Pag = 256 byte
一个扇区 = 16个Pag = 4096 byte
一个块 = 16个扇区 = 64KB
*/
typedef struct 
{///1 sector = 4096 byte 
    u8  databuf[4096];           //按找4096个数据进行读写设置
    u16 write_databuf_index;
    u32 write_index;            //记录当前芯片写入多少数据已经写入的总数据量
    u8  write_sector_index;     //写入扇区数量 //4096
    u32 read_index;             //记录当前芯片读取多少数据
    u8  read_sector_index;      //读取扇区数量 
}st_W25Q_Handler;       //当前读写 到哪里index的位置 数据量

/* Exported constants --------------------------------------------------------*/
/* Exported macro ------------------------------------------------------------*/
#define EXTERN_Flash

/* Exported functions ------------------------------------------------------- */
void W25Q64_Init(void);
u8 W25Q64_EraseChip(void);
u8 W25Q64_WriteData(u8 *data, u32 length);
u8 W25Q64_WriteData_End(void);
u8 W25Q64_ReadData(u8 *data, u16 *length);
#endif /* __FLASH_H */
