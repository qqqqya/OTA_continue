/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file W25Q_Handler.c
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
/* Includes ------------------------------------------------------------------*/
#include "w25qxx_Handler.h"
#include "w25qxx.h"
/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
static st_W25Q_Handler s_st_W25Q_Handler_1[2];
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/
/* extern variables ---------------------------------------------------------*/

/**
 * @brief 读取指定block的大小
 * 
 * @param block_index block的索引，0或1
 * @return uint32_t block的大小
 */
uint32_t Read_BlockSize(u8 block_index)
{
    return s_st_W25Q_Handler_1[block_index].write_index;
}
/* 
 * @brief 初始化W25Q64 初始化两个区域，A区 block0,B区 block1
 * 
 * @return void 
 */
void W25Q64_Init(void)
{
    W25Qx_Init();
    s_st_W25Q_Handler_1[0].read_index = 0;
    s_st_W25Q_Handler_1[0].read_sector_index = 0;
    s_st_W25Q_Handler_1[0].write_databuf_index = 0;
    s_st_W25Q_Handler_1[0].write_index = 0;
    s_st_W25Q_Handler_1[0].write_sector_index = 0;

    s_st_W25Q_Handler_1[1].read_index = 0;
    s_st_W25Q_Handler_1[1].read_sector_index = 0;
    s_st_W25Q_Handler_1[1].write_databuf_index = 0;
    s_st_W25Q_Handler_1[1].write_index = 0;
    s_st_W25Q_Handler_1[1].write_sector_index = 0;

}
/* 
 * @brief 擦除芯片的所有sector
 * 
 * @return u8 0:成功 1:失败
 */
u8 W25Q64_EraseChip(void)
{
    if(0 == W25Qx_Erase_Chip())
    {
    s_st_W25Q_Handler_1[0].read_index = 0;
    s_st_W25Q_Handler_1[0].read_sector_index = 0;
    s_st_W25Q_Handler_1[0].write_databuf_index = 0;
    s_st_W25Q_Handler_1[0].write_index = 0;
    s_st_W25Q_Handler_1[0].write_sector_index = 0;

    s_st_W25Q_Handler_1[1].read_index = 0;
    s_st_W25Q_Handler_1[1].read_sector_index = 0;
    s_st_W25Q_Handler_1[1].write_databuf_index = 0;
    s_st_W25Q_Handler_1[1].write_index = 0;
    s_st_W25Q_Handler_1[1].write_sector_index = 0;
        return 0;
    }
    return 1;
}
/* 
 * @brief 擦除指定block的sector
 * 
 * @param block_index block的索引，0或1
 * @return u8 0:成功 1:失败
 */
u8 Erase_Flash_Block(u8 block_index){

    s_st_W25Q_Handler_1[block_index].read_index = 0;
    s_st_W25Q_Handler_1[block_index].read_sector_index = 0;
    s_st_W25Q_Handler_1[block_index].write_databuf_index = 0;
    s_st_W25Q_Handler_1[block_index].write_index = 0;
    s_st_W25Q_Handler_1[block_index].write_sector_index = 0;
    return 0;
}

u8 W25Q64_WriteData(u8 block_index, u8 *data, u32 length)
{
    u8 ret = 0;
    u32 addr = 0;
    u16 index = 0;
    for(u16 i = 0; i < length; i++)
    {
        //1.数据写入数据缓冲区里面
        index = s_st_W25Q_Handler_1[block_index].write_databuf_index;
        s_st_W25Q_Handler_1[block_index].databuf[index] = *(data + i);
        s_st_W25Q_Handler_1[block_index].write_databuf_index++;
        //2.判断数据有没有写满4096
        if(s_st_W25Q_Handler_1[block_index].write_databuf_index == W25Qx_Para.SUBSECTOR_SIZE)
        {
            s_st_W25Q_Handler_1[block_index].write_databuf_index = 0;
            //擦除1个sector
            addr = W25Qx_Para.SUBSECTOR_SIZE * s_st_W25Q_Handler_1[block_index].write_sector_index;
            addr += block_index * BLOCK_SIZE;////////计算block的偏移地址
            W25Qx_Erase_Block(addr);
            W25Qx_WriteEnable();
            //写满了一个sector，执行写入操作   4096个byte
            for(u8 j = 0; j < 16; j++)///逐page写入
            {
                //获取当前写入地址
                addr = (W25Qx_Para.SUBSECTOR_SIZE * s_st_W25Q_Handler_1[block_index].write_sector_index) + \
                        (W25Qx_Para.PAGE_SIZE * j);
                        addr += block_index * BLOCK_SIZE;////////计算block的偏移地址
                //执行写入操作
                index = W25Qx_Para.PAGE_SIZE * j;
                W25Qx_Write(&s_st_W25Q_Handler_1[block_index].databuf[index],addr,W25Qx_Para.PAGE_SIZE);
            }
            s_st_W25Q_Handler_1[block_index].write_sector_index++;
            //记录总的写入数据长度
            s_st_W25Q_Handler_1[block_index].write_index += W25Qx_Para.SUBSECTOR_SIZE;
        }
    }
    return 0;
}

u8 W25Q64_WriteData_End(u8 block_index)
{
    u32 addr = 0;
    u16 index = 0;
    u8  page_size = 0;
    //判断还有没有剩余数据没有执行写入
    if(0 != s_st_W25Q_Handler_1[block_index].write_databuf_index)
    {
        //写入次数
        page_size = s_st_W25Q_Handler_1[block_index].write_databuf_index / W25Qx_Para.PAGE_SIZE;
        //先执行擦除扇区
        addr = W25Qx_Para.SUBSECTOR_SIZE * s_st_W25Q_Handler_1[block_index].write_sector_index;
        addr += block_index * BLOCK_SIZE;////////计算block的偏移地址

        W25Qx_Erase_Block(addr);
        W25Qx_WriteEnable();
        for(u8 j = 0; j < page_size; j++)//  page_size=0x02 0x280=640  
        {
            //获取当前写入地址
            addr = (W25Qx_Para.SUBSECTOR_SIZE * s_st_W25Q_Handler_1[block_index].write_sector_index) + \
                    (W25Qx_Para.PAGE_SIZE * j);
                addr += block_index * BLOCK_SIZE;////////计算block的偏移地址
            //执行写入操作
            index = W25Qx_Para.PAGE_SIZE * j;
            W25Qx_Write(&s_st_W25Q_Handler_1[block_index].databuf[index],addr,W25Qx_Para.PAGE_SIZE);
        }

        //有没有小于256的数据了
        if(0 != (s_st_W25Q_Handler_1[block_index].write_databuf_index % W25Qx_Para.PAGE_SIZE))
        {
            //获取当前写入地址
            addr = (W25Qx_Para.SUBSECTOR_SIZE * s_st_W25Q_Handler_1[block_index].write_sector_index) + \
                    (W25Qx_Para.PAGE_SIZE * page_size);
                    addr += block_index * BLOCK_SIZE;////////计算block的偏移地址
            //执行写入操作
            index = W25Qx_Para.PAGE_SIZE * page_size;
            W25Qx_Write(&s_st_W25Q_Handler_1[block_index].databuf[index],addr, \
                        s_st_W25Q_Handler_1[block_index].write_databuf_index % W25Qx_Para.PAGE_SIZE);
        }
        s_st_W25Q_Handler_1[block_index].write_index += s_st_W25Q_Handler_1[block_index].write_databuf_index;
    }
    return 0;
}

/*每次读取一个扇区的数据，外部接口需要一个4096的buffer
length:每次读取出来的长度，从缓冲区读取
return : 
        0:表示读取成功
        1:读取完毕  没有数据了
        2:读取失败  读取错误
*/
u8 W25Q64_ReadData(u8 block_index, u8 *data, u16 *length)
{
    u8 ret = 0;
    u32 addr = 0;
    u16 index = 0;
    u8  page_size = 0;
    u16  remain_size = 0;

    //1.先判断是否读取完毕
    if(s_st_W25Q_Handler_1[block_index].write_index > s_st_W25Q_Handler_1[block_index].read_index)
    {
        //判断下数据是否够4K
        if(s_st_W25Q_Handler_1[block_index].write_sector_index > s_st_W25Q_Handler_1[block_index].read_sector_index)
        {
            //读4K数据的操作
            *length = W25Qx_Para.SUBSECTOR_SIZE;//4k size
            addr = s_st_W25Q_Handler_1[block_index].read_sector_index * W25Qx_Para.SUBSECTOR_SIZE;
            addr += block_index * BLOCK_SIZE;////////计算block的偏移地址
            if(0 != W25Qx_Read(data,addr,*length))
                return 2;
            s_st_W25Q_Handler_1[block_index].read_sector_index++;
        }
        else
        {
            //读4K以内的数据--write_index - read_index 
            *length = s_st_W25Q_Handler_1[block_index].write_index - s_st_W25Q_Handler_1[block_index].read_index;
            addr = s_st_W25Q_Handler_1[block_index].read_sector_index * W25Qx_Para.SUBSECTOR_SIZE;
                addr += block_index * BLOCK_SIZE;////////计算block的偏移地址
            if(0 != W25Qx_Read(data,addr,*length))
                return 2;
        }
        s_st_W25Q_Handler_1[block_index].read_index += *length;
        return 0;
    }
    else
    {
        return 1;
    }
}
