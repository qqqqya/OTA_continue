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
 * @version V1.0 2026年7月29日
 *
 * @note 1 tab == 4 spaces!
 * 
 *****************************************************************************/
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __SFUD_ADAPTER_H
#define __SFUD_ADAPTER_H

#include "main.h"
#include "inc/sfud.h"

// 保持与原有接口兼容的宏定义
#define BLOCK_1         0
#define BLOCK_2         1
#define BLOCK_SIZE      0x10000

// 函数声明
void SFUD_W25Q64_Init(void);
uint8_t SFUD_W25Q64_EraseChip(void);
void SFUD_Erase_Flash_Block(uint8_t block_index);
uint8_t SFUD_W25Q64_WriteData(uint8_t block_index, uint8_t *data, uint32_t length);
uint8_t SFUD_W25Q64_WriteData_End(uint8_t block_index);
uint8_t SFUD_W25Q64_ReadData(uint8_t block_index, uint8_t *data, uint16_t *length);
void SFUD_SetBlockParmeter(uint8_t block_index, uint32_t app_size);
uint32_t SFUD_Read_BlockSize(uint8_t block_index);

#endif /* __SFUD_ADAPTER_H */
