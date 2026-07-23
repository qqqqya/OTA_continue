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
#include "flash.h"
#include "at24cxx_driver.h"
#include "elog.h"
#include "w25qxx_Handler.h"
#include "ymodem.h"


typedef void (*pFunc)(void);      //pFunc 是变量名'，类型是 void (*)(void)。
// pFunc Jump2Application;//函数指针类型--变量 
                        //完全等价与 void (*Jump2Application)(void) 
  pFunc Jump2Application;//函数指针类型--变量 
                        //完全等价与 void (*Jump2Application)(void) 

  uint32_t JumpAddress;//跳转地址
uint16_t app_size = 0;//APP程序大小

uint8_t au8_test[1024]; //测试数据缓存

///向量--密钥  1(dec) 0x31(ascii)  2(dec) 0x32(ascii)
unsigned char IV[16]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};  
unsigned char Key[32]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,\
                       0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};
u8  Mem_Read_buffer[4096];//读4k  外部flash读数据缓存

void Ota_statemanage(void){
  uint8_t ota_status = NO_APP_UPDATE;//OTA状态
  int32_t fil_size = 0;//文件大小
  uint32_t t_u32_appsize = 0;//APP程序大小

  ee_ReadBytes(&ota_status,0x00,1);//读取OTA状态字节
  switch(ota_status)
  {
    case NO_APP_UPDATE://没有更新的情况下，如果按下就重新拷贝，如果没按下就直接跳转APP程序
      if(Key_Scan())//通常情况下，都是直接跳转APP程序
      {   //按下         
        /*1.将接收到的数据 拷贝到exflash A区*/
        fil_size = Ymodem_Receive(au8_test);
        /*2.解密数据到exflash B区*/
        if(0 == ExA_To_ExB_AES(fil_size))
        {
        /*3.拷贝当前的APP程序 到exflash A区--为了回滚使用*/
          ee_ReadBytes((uint8_t *)&t_u32_appsize,0x05,4);//app程序大小
          App_To_ExA(t_u32_appsize);//0X05存的是当前APP字节大小
        /*4.exflash B区的解密后数据加载到片上flash*/
          ExB_To_App();
        /*5.跳转APP程序*/
          Jump2App();
    //这里其实就是如果跳转不成功，就将上一次备份的能运行的程序再一次的拷贝回片上flash中
          // /*6.如果运行到这一步，说明数据无效，把外部A区的数据搬运到App中*/
          ExA_To_App();
          /*再执行一次跳转*/
          Jump2App();
        }
        else
        {
          //解密不成功，直接跳转APP程序，不进行其他操作
          log_e("Download Failed!");
          Jump2App();
        }
      }
      else
      {
        Jump2App();//KEY_Scan() 没有按下，直接跳转APP程序
      }
      break;
    case APP_DOWNLOADING://0x11--EEPROM--说明，只是进行了协议握手，但是没有传输成功文件
      log_a("App dowload failed");//1、如果下载不成功的话，就会卡在11这个状态码上
      Jump2App();                 //说明是APP工程，下载的时候变失败
      log_a("NO VALID APP!!");//2、ApP地址上没有固件/数据错误，要重新下载固件
      /*2.1 没有传输成功文件，那么就进行跳转到上一个版本的固件，如果跳转固件失败，就说明固件受损。
      所以大多数情况下都只是上面的这两句，下面的这些会很少触发*/
      //3、接收数据，然后后面进行跳转
      fil_size = Ymodem_Receive(au8_test);
       if(0 == ExA_To_ExB_AES(fil_size))
        {
        /*3.拷贝当前的APP程序 到exflash A区--为了回滚使用*/
          ee_ReadBytes((uint8_t *)&t_u32_appsize,0x05,4);//app程序大小
          App_To_ExA(t_u32_appsize);//0X05存的是当前APP字节大小
        /*4.exflash B区的解密后数据加载到片上flash*/
          ExB_To_App();
        /*5.跳转APP程序*/
          Jump2App();
    //这里其实就是如果跳转不成功，就将上一次备份的能运行的程序再一次的拷贝回片上flash中
          // /*6.如果运行到这一步，说明数据无效，把外部A区的数据搬运到App中*/
          ExA_To_App();
          /*再执行一次跳转*/
          Jump2App();
        }
        else
        {
          //解密不成功，直接跳转APP程序，不进行其他操作
          log_e("Download Failed!");
          Jump2App();
        }
      break;

    case APP_DOWNLOAD_COMPLETE://0x22--说明，更新完成，把上个版本的估计被分到ex flash, 方便回滚
      // 读取当前需要更新的App的大小--4字节大小；从01地址开始读
        //0X 01存的是新APP的字体大小，0X05存的是当前APP字节大小
      ee_ReadBytes((uint8_t *)&t_u32_appsize,0x01,4);
      /*1.App大小更新到--内部flash存储数据量的管理结构体中*/
        //也就是flash的index
        W25Q64_SetBlockIndex(BLOCK_1,t_u32_appsize);//更新 区的index
      /*2.解密数据到exflash B区*/ //因为之前的数据已经从APP的过程中接收成功
        if(0 == ExA_To_ExB_AES(t_u32_appsize))//--//APP下载成功这个分支里面说明是，APP在进行接收固件包，所以局部变量 `fil_size` 的值一直是初始值 
        { /*之前用的fil_size；一直是初始化值，在上面这个判断里面直接会返回-1，
              解密失败，然后就会跳回到之前的固件上*/
      /*3.拷贝当前的APP程序 到exflash A区--为了回滚使用*/
          ee_ReadBytes((uint8_t *)&t_u32_appsize,0x05,4);//app程序大小
          App_To_ExA(t_u32_appsize);              //0X05存的是当前APP字节大小
        /*4.exflash B区的解密后数据加载到片上flash*/
          ExB_To_App();
        /*5.跳转APP程序*/
          Jump2App();
    //这里其实就是如果跳转不成功，就将上一次备份的能运行的程序再一次的拷贝回片上flash中
          // /*6.如果运行到这一步，说明数据无效，把外部A区的数据搬运到App中*/
          ExA_To_App();
          /*再执行一次跳转*/
          Jump2App();
        }
        else
        {
          //解密不成功，直接跳转APP程序，不进行其他操作
          log_e("BOOT Download Failed!");
          Jump2App();
        }

      
      break;
    default:

      /*NO action*/
      break;
  }
}
/**
 * @brief 解密数据到B区---将外部flash A区的数据搬运到外部flash B区并进行AES解密
 * 
 * @param fl_size 
 * @return int8_t 
 */
int8_t ExA_To_ExB_AES(int32_t fl_size){
    u8 Temp[16];  //原密文数据缓存
    u16 readTime=0,readDataCount=0;   //读取数据再解密的次数（每次解密16个字节）
    u32 AppSize=0;  //升级包的大小
    //u32 FlashDestination=ApplicationAddress;
    u16 Read_Memory_Size=0;
    u32 Read_Memory_index=0;
    uint8_t *pu8_IV_IN_OUT = IV;
    uint8_t *pu8_key256bit = Key;

    if(fl_size <= 0)
    {
      return -1;
    }
    if ((app_size > (0x18010 - 1)) ||\
    (app_size < 0))//这里的96kb表示  划分的APP运行区就是96 KB
    {//appsize 在ymodemn.c中已经解析出来了
      return -1;
    }
    //先读一帧，用来解析头文件格式
    W25Q64_ReadData(BLOCK_1,Mem_Read_buffer,&Read_Memory_Size);
    if(Read_Memory_Size  >= 16)
    {
      memcpy(Temp,Mem_Read_buffer,16);
      Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,Temp,pu8_key256bit);
      // 通过向量-密文-密钥-解密得到明文
      // 这里的密文，是一帧一帧解密的，像这里就是16个字节
      // 先解析密文大小,得出要解析的次数，后面的密文解析是一帧一帧的去
      AppSize=(Temp[15]<<24)+(Temp[14]<<16)+(Temp[13]<<8)+Temp[12];
      log_d("AppSize=%d",AppSize);
      
      //计算升级包读取次数
      readDataCount=AppSize/16;
      if(AppSize%16!=0)
      {
        readDataCount+=1;
      }
      Read_Memory_index += 16;
    }
    else
    {
      return -1;
    }

    //数据帧
    //擦除外部flash比较耗时--这里先不擦出、对于A区在擦除
        // uint8_t flash_erase_state = Flash_erase(AppRunFlashDestination,AppSize);

      for(readTime=0;readTime<readDataCount;readTime++)
      {
        //判断下当前buffer下的数据是否读取完毕
        if(Read_Memory_index == Read_Memory_Size)
        {
          if(2 == W25Q64_ReadData(BLOCK_1,Mem_Read_buffer,&Read_Memory_Size))
          {//2:读取失败  0:读取成功  1:读取成功
            return -1;
          }
          Read_Memory_index = 0;
        }
        //拷贝16个数据
        memcpy(Temp,Mem_Read_buffer + Read_Memory_index,16);
        Read_Memory_index += 16;
        //解析16个数据
        Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,Temp,pu8_key256bit);//解析

        //写入到外部flash B区
        W25Q64_WriteData(BLOCK_2,Temp,16);
        // for (wirteTime = 0;wirteTime<4;wirteTime++)
        // {
        //   Flash_Write(AppRunFlashDestination, *(uint32_t*)RamSource);
        //   AppRunFlashDestination += 4;
        //   RamSource += 4;
        // }
      }
    W25Q64_WriteData_End(BLOCK_2);
    log_d("Write_Flash_After_AES_Decode end");
    return 0;
}
/**
 * @brief 将当前的APP程序 到exflash A区--为了回滚使用
 * 
 * @param fl_size 
 */
int8_t App_To_ExA(int32_t fl_size){
  u32 flash_des=ApplicationAddress;

  if ((fl_size > (0x18010 - 1)) ||\
  (fl_size < 0))//这里的96kb表示  划分的APP运行区就是96 KB
  {//appsize 在ymodemn.c中已经解析出来了
    return -1;
  }
  Erase_Flash_Block(BLOCK_1);//擦除A区--只是清除结构体
  W25Q64_WriteData(BLOCK_1,(u8 *)flash_des,fl_size);//将当前的APP程序搬运到外部flash A区
  W25Q64_WriteData_End(BLOCK_1);
  
  return 0;
}
/**
 * @brief 将解密过后的外部flash B区的数据搬运到片上flash中
 * 
 * @return int8_t 
 */
/*wrong version*/
int8_t ExB_To_App(void){

  u32 flash_des=ApplicationAddress;
  // u8 flash_des=ApplicationAddress;
  u32 RamSource = 0;
  uint16_t write_time=0;
  u16 Read_Memory_Size=0;
  u32 Read_Memory_index=0;

  u32 flash_size = 0;
  u32 read_datastate = 0;
  /*1.读取外部flash B区的大小 擦除片上flash的对应区域*/
  flash_size = Read_BlockSize(BLOCK_2);
  if( 0== Flash_erase(flash_des,flash_size)){
    //擦除成功
    for(;;){
      /*2.读取外部flash B区数据 到Mem_Read_buffer临时buf*/
      read_datastate = W25Q64_ReadData(BLOCK_2,Mem_Read_buffer,&Read_Memory_Size);
      if(read_datastate == 1){//读取完毕 退出循环
        break;
      }
      else if(read_datastate == 2){//读取失败 退出循环
        break;
      }
      /*3.写数据 将数据从Mem_Read_buffer临时buf搬运到片上flash中*/
      else{//读取成功--将数据搬运到片上flash
        RamSource = (uint32_t)Mem_Read_buffer;
        for(write_time = 0; write_time < (Read_Memory_Size/4);write_time++)
        {/*3.1.四字节的搬运到片上flash*/
          // Flash_Write(flash_des,RamSource);
          Flash_Write(flash_des,*(uint32_t *)RamSource);
          flash_des += 4;
          RamSource += 4;
        }    
      }
    }
    return flash_size;//返回搬运的大小
  }else{
    //擦除失败
    return -1;
  }
  return 0;
}
/**
 * @brief 就是将上一次备份的能运行的程序再一次的拷贝回片上flash中
 * 将外部flash A区的数据搬运到片上flash中
 * 
 * @param fl_size 
 */
int8_t ExA_To_App(void)
{
  u32 FlashDes = ApplicationAddress;
  u32 flashsize = 0;
  u8  Read_dataState = 0;
  u16 Read_Memorysize = 0;
  u32 RamSource = 0;
  u16 writeTime=0;
  /*擦除本地flash数据*/
  flashsize = Read_BlockSize(BLOCK_1);
  if(1 == Flash_erase(FlashDes,flashsize))
  {
    return -1;
  }
  else
  {
    for(;;)
    {
      Read_dataState = W25Q64_ReadData(BLOCK_1,Mem_Read_buffer,&Read_Memorysize);
      if(1 == Read_dataState)
      {
        //数据读完退出
        break;
      }
      else if(2 == Read_dataState)
      {
        //外部flash数据读取有问题
        break;
      }
      else
      {
        RamSource = (u32)Mem_Read_buffer;
        //循环搬运flash数据
        for(writeTime = 0; writeTime < (Read_Memorysize/4);writeTime++)
        {
          Flash_Write(FlashDes,*(uint32_t *)RamSource);
          FlashDes += 4;
          RamSource += 4;
        }
      }
    }
    return flashsize;
  }
}

#if 0
int8_t ExA_To_App(void){
  u8 flash_des=ApplicationAddress;
  u32 RamSource = 0;
  uint16_t write_time=0;   //写入数据次数

  u16 Read_Memory_Size=0;
  u32 Read_Memory_index=0;

  u32 flash_size = 0;
  u32 read_datastate = 0;
  flash_size = Read_BlockSize(BLOCK_1);//读取A区的大小
  if( 0== Flash_erase(flash_des,flash_size)){

    for(;;){
      //读取外部flash A区数据
      read_datastate = W25Q64_ReadData(BLOCK_1,Mem_Read_buffer,&Read_Memory_Size);
      if(read_datastate == 1){//读取完毕 退出循环
        break;
      }
      else if(read_datastate == 2){//读取失败 退出循环
        break;
      }
      else{//读取成功--将数据搬运到片上flash
        RamSource = (uint32_t)Mem_Read_buffer;
        for(write_time = 0; write_time < (Read_Memory_Size/4);write_time++)
        {//四字节的搬运
          Flash_Write(flash_des,RamSource);
          flash_des += 4;
          RamSource += 4;
        }
      }
    }
    return flash_size;//返回搬运的大小
  }
  else{
    //擦除失败
    return -1;
  }
}
  

int8_t External_AES_Backup2App(int32_t fl_size){
    u8 Temp[16];  //原密文数据缓存
    u8 wirteTime=0;  //一个解析包写入次数
    u16 readTime=0,readDataCount=0;   //读取数据再解密的次数（每次解密16个字节）
    u32 AppSize=0;  //升级包的大小
    //u32 FlashDestination=ApplicationAddress;
    u16 Read_Memory_Size=0;
    u32 Read_Memory_index=0;
    uint8_t *pu8_IV_IN_OUT = IV;
    uint8_t *pu8_key256bit = Key;

    
    uint32_t RamSource = 0;
    uint32_t AppRunFlashDestination = ApplicationAddress;
    if(fl_size <= 0)
    {
      return -1;
    }
    if ((app_size > (0x18010 - 1)) ||\
    (app_size < 0))
    {
      return -1;
    }
    //先读一帧，用来解析头文件格式
    W25Q64_ReadData(Mem_Read_buffer,&Read_Memory_Size);
    if(Read_Memory_Size  >= 16)
    {
      memcpy(Temp,Mem_Read_buffer,16);
      Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,Temp,pu8_key256bit);
      // 通过向量-密文-密钥-解密得到明文
      // 这里的密文，是一帧一帧解密的，像这里就是16个字节
      // 先解析密文大小,得出要解析的次数，后面的密文解析是一帧一帧的去
      AppSize=(Temp[15]<<24)+(Temp[14]<<16)+(Temp[13]<<8)+Temp[12];
//      log_d("AppSize=%d",AppSize);
      
      //计算升级包读取次数
      readDataCount=AppSize/16;
      if(AppSize%16!=0)
      {
        readDataCount+=1;
      }
      Read_Memory_index += 16;
    }

    //数据帧
    //将待写入区的内容擦除
    uint8_t flash_erase_state = Flash_erase(AppRunFlashDestination,AppSize);
    if(flash_erase_state == 0)
    {
      for(readTime=0;readTime<readDataCount;readTime++)
      {
        //判断下当前buffer下的数据是否读取完毕
        if(Read_Memory_index == Read_Memory_Size)
        {
          if(2 == W25Q64_ReadData(Mem_Read_buffer,&Read_Memory_Size))
          {//2:读取失败  0:读取成功  1:读取成功
            return -1;
          }
          Read_Memory_index = 0;
        }
        //拷贝16个数据
        memcpy(Temp,Mem_Read_buffer + Read_Memory_index,16);
        Read_Memory_index += 16;
        //解析16个数据
        Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,Temp,pu8_key256bit);//解析
        
        RamSource = (uint32_t)Temp;
        for (wirteTime = 0;wirteTime<4;wirteTime++)
        {
          Flash_Write(AppRunFlashDestination, *(uint32_t*)RamSource);
          AppRunFlashDestination += 4;
          RamSource += 4;
        }
      }
      return 0;
    }
    else
    {
      return -1;
    }
#endif

#if 0

  /* 1、位置？外部flash  写入数据 */
  // uint8_t *pu8_temp = (uint8_t *)BackupApplicationAddress;  //原始数据//备份APP地址  08020000
  uint8_t Temp[16];  //原密文数据缓存
  u32 AppSize=0;  //升级包的大小
  u8 wirteTime=0;  //一个解析包写入次数----------
  uint16_t readTime=0,readDataCount=0;   //读取数据再解密的次数（每次解密16个字节

    u16 Read_Memory_Size=0;
    u32 Read_Memory_index=0;
    uint32_t RamSource = 0;
    uint32_t FlashDestination = ApplicationAddress;//APP地址        08008000
    
  if(fl_size <= 0)return -1;

  if(app_size < 0  || app_size > (0x18010 - 1)){//96k + 16字节
        printf("app size error\r\n");
        return -1;
  }//先读一帧，用来解析头文件格式
  W25Q64_ReadData(Mem_Read_buffer,&Read_Memory_Size);

      if(Read_Memory_Size  >= 16)
    {  
        memcpy(Temp,Mem_Read_buffer,16);//缓存原密文数据--Mem_Read_buffer-----copy 16个字节出来
        //解析得到自定义内容+文件大小
      Aes_IV_key256bit_Decode(IV, Temp, Key);//向量输入  pTemp（密文输入  明文输出） 秘钥
      AppSize=(Temp[15]<<24)+(Temp[14]<<16)+(Temp[13]<<8)+ Temp[12]; //铭文APP大小

      /*计算需要解密多少次*/
      readDataCount=AppSize/16;
      if(AppSize%16!=0)
      {
        readDataCount+=1;
      }
      Read_Memory_index += 16;//指向下一帧数据
    }

  /* 2、还是需要擦出的 因为app是内部flash */
  //擦除运行区数据
  if(1 == Flash_erase(FlashDestination,AppSize))
  {
    return -1;
  }
  /* 3、剩余的好像就不用改了把 */
  //读数据的总次数
  for(readTime=0;readTime<readDataCount;readTime++) //325次 
  {
    /* 3.1//判断下当前buffer下的数据是否读取完毕 */
        if(Read_Memory_index == Read_Memory_Size)
        {
          if(2 == W25Q64_ReadData(Mem_Read_buffer,&Read_Memory_Size))
          {
//            log_d("Write_Flash_After_AES_Decode read extern buffer error");
            return -1;
          }
          Read_Memory_index = 0;
        }
    //加密原文读取16个字节到临时区中
    // pTemp = Temp;
    // memcpy(pTemp,pu8_temp,16);
    // pu8_temp += 16;
        memcpy(Temp,(Mem_Read_buffer + Read_Memory_index),16);
        Read_Memory_index += 16;
    Aes_IV_key256bit_Decode(IV,Temp,Key);//解密数据 

        RamSource = (uint32_t)Temp;//转存为32位地址
        for (wirteTime = 0;wirteTime<4;wirteTime++)
        {
          Flash_Write(FlashDestination, *(uint32_t*)RamSource);
          FlashDestination += 4;
          RamSource += 4;
        }
    // //解密后的数据存入App运行区中   pTemp
    // for (uint8_t j = 0;j < 16; j+= 4) //每次16个字节  写入4个字节 循环readDataCount次
    // {
    //   Flash_Write(FlashDestination,*(uint32_t*)pTemp);
    //   if (*(uint32_t*)FlashDestination != *(uint32_t*)pTemp)
    //   {
    //     return -1;
    //   }
    //   FlashDestination += 4;
    //   pTemp += 4;
    // }
  }
  return 0;
#endif

int8_t AES_Backup2App(int32_t fl_size){
    uint32_t FlashDestination = ApplicationAddress;//APP地址        08008000

  uint8_t *pu8_temp = (uint8_t *)BackupApplicationAddress;  //原始数据//备份APP地址  08020000
  uint8_t Temp[16];  //原密文数据缓存
  uint8_t *pTemp = Temp;
  u32 AppSize=0;  //升级包的大小
  uint16_t readTime=0,readDataCount=0;   //读取数据再解密的次数（每次解密16个字节
  
  if(fl_size <= 0)return -1;

  if(app_size < 0  || app_size > (0x18010 - 1)){//96k + 16字节
        printf("app size error\r\n");
        return -1;
  }
  memcpy(pTemp,pu8_temp,16);//缓存原密文数据-------copy 16个字节出来
    Aes_IV_key256bit_Decode(IV, pTemp, Key);//向量输入  pTemp（密文输入  明文输出） 秘钥
  AppSize=(pTemp[15]<<24)+(pTemp[14]<<16)+(pTemp[13]<<8)+pTemp[12]; //铭文APP大小
  pu8_temp += 16;       //指针指向下一个16字节

  /*计算需要解密多少次*/
  readDataCount=AppSize/16;
  if(AppSize%16!=0)
  {
    readDataCount+=1;
  }

  //擦除运行区数据
  if(1 == Flash_erase(FlashDestination,AppSize))
  {
    return -1;
  }

  //读数据的总次数
  for(readTime=0;readTime<readDataCount;readTime++) //325次 
  {
    //加密原文读取16个字节到临时区中
    pTemp = Temp;
    memcpy(pTemp,pu8_temp,16);
    pu8_temp += 16;
    Aes_IV_key256bit_Decode(IV,pTemp,Key);//解密数据 
    //解密后的数据存入App运行区中   pTemp
    for (uint8_t j = 0;j < 16; j+= 4) //每次16个字节  写入4个字节 循环readDataCount次
    {
      Flash_Write(FlashDestination,*(uint32_t*)pTemp);
      if (*(uint32_t*)FlashDestination != *(uint32_t*)pTemp)
      {
        return -1;
      }
      FlashDestination += 4;
      pTemp += 4;
    }
  }
    return 0;
}

 void Jump2App(void){

     /* 检查栈顶地址是否合法 */
     if(((*(__IO uint32_t *)ApplicationAddress) & 0x2FFE0000) == 0x20000000)
     {
// 		printf("jump addr start\r\n");  
         /* 屏蔽所有中断，防止在跳转过程中，中断干扰出现异常 */
         __disable_irq();
         NVIC_SetVectorTable(FALSH_BASE_ADDR, 0x8000);
         RCC_DeInit();
         /* 用户代码区第二个 字 为程序开始地址(复位地址) */
         JumpAddress = *(__IO uint32_t *) (ApplicationAddress + 4);
                  // 		printf("jump addr :0x%08X\r\n",JumpAddress);
         /* Initialize user application's Stack Pointer */
         /* 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址) */
         __set_MSP(*(__IO uint32_t *) ApplicationAddress);///将8000的MSP重新设置
		
         /* 类型转换 */
         Jump2Application = (pFunc) JumpAddress;
 /*这句话在汇编层面其实就是把0x080081AD 塞进了 PC(程序计数器)
 PC指针瞬间来到了 0x080081AC，这里存放的是APP工程里的
 Reset_Handler 汇编代码。*/
         /* 跳转到 APP */
         Jump2Application();
     }
   }

/**将back写入app地址
 * 先提取back 再将back写入app地址--erase app地址--write back到app地址
 */
int8_t Backup2App(void){
    uint32_t FlashDestination = ApplicationAddress;//APP地址        08008000
    uint32_t BackupSource = BackupApplicationAddress;//备份APP地址  08020000
    uint32_t j = 0;

    if(app_size < 0  || app_size > (0x18000 - 1)){//APP地址0x08008000-0x08020000 96k
        printf("app size error\r\n");
        return -1;
    }
    ///copy back to app
    for (j = 0;j < app_size;j += 4) //每次写入4个字节
    {
      Flash_Write(FlashDestination, *(uint32_t*)BackupSource);//destin 是app地址
          /* Check the written data */
      if (*(uint32_t*)FlashDestination != *(uint32_t*)BackupSource)
      {/***  *(uint32_t*)0x08008000 → 读取刚写入的主 APP 区数据--------*(uint32_t*)0x08020000 → 备份区原始数据 */
        return -2;
      }
      FlashDestination += 4;
      BackupSource += 4;
    }
    return 0;
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
