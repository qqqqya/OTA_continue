/*include*/
#include "manage_jmp.h"
#include "main.h"
#include "Flash.h"
#include "AES.h"
#include "w25qxx_Handler.h"
#include "w25qxx.h"
#include "elog.h"
#include "AT24Cxx_Driver.h"
#include "Gpio.h"
#include "ymodem.h"
/*include*/



uint8_t au8_test[1024]; 

uint32_t JumpAddress;

u8  Mem_Read_buffer[4096];
typedef void (*pFunction)(void);      //pFunc 是变量名'，类型是 void (*)(void)。

void JumpToApp(void)
{
    /* 检查栈顶地址是否合法 */
    uint32_t JumpAddress;
    pFunction Jump_To_Application;

    /* 检查栈顶地址是否合法 */
    if(((*(__IO uint32_t *)ApplicationAddress) & 0x2FFE0000) == 0x20000000)
    {
     printf("jump addr start\r\n"); 
        /* 屏蔽所有中断，防止在跳转过程中，中断干扰出现异常 */
        __disable_irq();
        NVIC_SetVectorTable(NVIC_VectTab_FLASH, 0x8000);
        RCC_DeInit();
        /* 用户代码区第二个 字 为程序开始地址(复位地址) */
        JumpAddress = *(__IO uint32_t *) (ApplicationAddress + 4);
      printf("jump addr start\r\n"); 
        /* Initialize user application's Stack Pointer */
        /* 初始化APP堆栈指针(用户代码区的第一个字用于存放栈顶地址) */
        __set_MSP(*(__IO uint32_t *) ApplicationAddress);

        /* 类型转换 */
        Jump_To_Application = (pFunction) JumpAddress;

        /* 跳转到 APP */
        Jump_To_Application();
    }
}

int32_t app_size = 0;

unsigned char IV[16]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};  
unsigned char Key[32]={0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,\
                       0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32,0x31,0X32};

/**
 * @brief OTA状态机处理
 * 
 * @param None
 * 
 * @return: None
 */
void Ota_statemanage(void)
{
  uint8_t t_u8_otastate = NO_APP_UPDATE;
  uint32_t t_u32_appsize = 0;
	int32_t fil_size = 0;
  /*读eeprom的OTA状态*/
  ee_ReadBytes(&t_u8_otastate,0x00,1);

  switch(t_u8_otastate)
  {
    case NO_APP_UPDATE:
      if(Key_Scan())
      {
        //按下
        /*1.下载到外部flash的A区*/
        fil_size = Ymodem_Receive(au8_test);
        /*2.解密数据拷贝到外部flashB区中*/
        if(0 == ExA_To_ExB_AES(fil_size))
        {
          /*3.当前App的数据搬运到外部flash的A区 ，增加一个代码回退功能*/
          //读当前App的大小
          ee_ReadBytes((uint8_t *)&t_u32_appsize,0x05,4);
          App_To_ExA(t_u32_appsize);
          /*4.外部flashB区的数据搬运到内部App区域中*/
          ExB_To_App();
          /*5.执行跳转逻辑*/
          JumpToApp();
          /*6.如果运行到这一步，说明数据无效，把外部A区的数据搬运到App中*/
          ExA_To_App();
          /*再执行一次跳转*/
          JumpToApp();
        }
        else
        {
          log_a("Boot dowload failed");
          JumpToApp();
        }
      }
      else
      {
        JumpToApp();
      }
    break;

    case APP_DOWNLOADING:
      log_a("App dowload failed");
      JumpToApp();
      //1.跳转失败，下载新App
      log_a("No Valid App");

      fil_size = Ymodem_Receive(au8_test);
      /*2.解密数据拷贝到外部flashB区中*/
      if(0 == ExA_To_ExB_AES(fil_size))
      {
        /*3.当前App的数据搬运到外部flash的A区 ，增加一个代码回退功能*/
        //读当前App的大小
        ee_ReadBytes((uint8_t *)&t_u32_appsize,0x05,4);
        App_To_ExA(t_u32_appsize);
        /*4.外部flashB区的数据搬运到内部App区域中*/
        ExB_To_App();
        /*5.执行跳转逻辑*/
        JumpToApp();
        /*6.如果运行到这一步，说明数据无效，把外部A区的数据搬运到App中*/
        ExA_To_App();
        /*再执行一次跳转*/
        JumpToApp();
      }
      else
      {
        log_a("Boot dowload failed");
        JumpToApp();
      }
    break;

    case APP_DOWNLOAD_COMPLETE:
      //1.读取当前需要更新的App的大小
      ee_ReadBytes((uint8_t *)&t_u32_appsize,0x01,4);
      //2.App大小更新到内部flash存储数据量的管理结构体中
      // SetBlockParmeter(BLOCK_1,t_u32_appsize);
      /*2.解密数据拷贝到外部flashB区中*/
      if(0 == ExA_To_ExB_AES(t_u32_appsize))
      {
        /*3.当前App的数据搬运到外部flash的A区 ，增加一个代码回退功能*/
        //读当前App的大小
        ee_ReadBytes((uint8_t *)&t_u32_appsize,0x05,4);
        App_To_ExA(t_u32_appsize);
        /*4.外部flashB区的数据搬运到内部App区域中*/
        ExB_To_App();
        /*5.执行跳转逻辑*/
        JumpToApp();
        /*6.如果运行到这一步，说明数据无效，把外部A区的数据搬运到App中*/
        ExA_To_App();
        /*再执行一次跳转*/
        JumpToApp();
      }
      else
      {
        log_a("Boot dowload failed");
        JumpToApp();
      }
    break;

    default:
    /*No action*/
    break;
  }
}

/**
 * @brief 外部flashA区中数据拷贝到App区中
 * 
 * @param None
 * 
 * @return: The status of the initialization.
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
          Flash_Write(FlashDes,RamSource);
          FlashDes += 4;
          RamSource += 4;
        }
      }
    }
    return flashsize;
  }
}

/**
 * @brief 外部flashB区中数据拷贝到A区中
 * 
 * @param None
 * 
 * @return: The status of the initialization.
 */
int8_t ExB_To_App(void)
{
  u32 FlashDes = ApplicationAddress;
  u32 flashsize = 0;
  u8  Read_dataState = 0;
  u16 Read_Memorysize = 0;
  u32 RamSource = 0;
  u16 writeTime=0;
  /*擦除本地flash数据*/
  flashsize = Read_BlockSize(BLOCK_2);
  if(1 == Flash_erase(FlashDes,flashsize))
  {
    return -1;
  }
  else
  {
    for(;;)
    {
      Read_dataState = W25Q64_ReadData(BLOCK_2,Mem_Read_buffer,&Read_Memorysize);
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
          //Flash_Write(FlashDes,RamSource);//Old
          Flash_Write(FlashDes,*(u32 *)RamSource);
          FlashDes += 4;
          RamSource += 4;
        }
      }
    }
    return flashsize;
  }
}

/**
 * @brief 外部flashA区数据拷贝到外部flashB区中
 * 
 * @param None
 * 
 * @return: The status of the initialization.
 */
int8_t App_To_ExA(int32_t fl_size)
{
  u32 FlashDes = ApplicationAddress;
  if ((fl_size > (0x18010 - 1)) ||\
  (fl_size < 0))
  {
    return -1;
  }

  Erase_Flash_Block(BLOCK_1);
  W25Q64_WriteData(BLOCK_1,(uint8_t *)FlashDes,fl_size);
  W25Q64_WriteData_End(BLOCK_1);
	return 0;
}

/**
 * @brief 外部flashA区数据拷贝到外部flashB区中
 * 
 * @param None
 * 
 * @return: The status of the initialization.
 */
int8_t ExA_To_ExB_AES(int32_t fl_size)
{
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
  (app_size < 0))
  {
    return -1;
  }
  //先读一帧，用来解析头文件格式
  W25Q64_ReadData(BLOCK_1, Mem_Read_buffer,&Read_Memory_Size);
  if(Read_Memory_Size  >= 16)
  {
    memcpy(Temp,Mem_Read_buffer,16);
    Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,Temp,pu8_key256bit);//解析得到自定义内容+文件大小
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

  //擦除外部flash比较耗时
  for(readTime=0;readTime<readDataCount;readTime++)
  {
    //判断下当前buffer下的数据是否读取完毕
    if(Read_Memory_index == Read_Memory_Size)
    {
      if(2 == W25Q64_ReadData(BLOCK_1, Mem_Read_buffer,&Read_Memory_Size))
      {
        log_d("Write_Flash_After_AES_Decode read extern buffer error");
        return -1;
      }
      Read_Memory_index = 0;
    }
    //拷贝16个数据
    memcpy(Temp,Mem_Read_buffer + Read_Memory_index,16);
    Read_Memory_index += 16;
    //解析16个数据
    Aes_IV_key256bit_Decode(pu8_IV_IN_OUT,Temp,pu8_key256bit);//解析
    
    W25Q64_WriteData(BLOCK_2,Temp,16);
  }
  W25Q64_WriteData_End(BLOCK_2);
  log_d("Write_Flash_After_AES_Decode end");
  return 0;
}
