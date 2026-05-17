/******************************************************************************
 * Copyright (C) 2024 EternalChip, Inc.(Gmbh) or its affiliates.
 * 
 * All Rights Reserved.
 * 
 * @file ymodem.c
 * 
 * @par dependencies 
 * - ymodem.h
 * 
 * @author Simon | R&D Dept. | EternalChip 立芯嵌入式
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
#include "ymodem.h"

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
uint8_t file_name[FILE_NAME_LENGTH];
uint32_t RamSource;

extern QueueHandle_t Q_YmodemReclength;

extern QueueHandle_t Queue_AppDataBuffer;/// 下载buf切换---队列
extern SemaphoreHandle_t Semaphore_ExtFlashState;/// 外部flash---互斥量

static uint16_t s_u16_YmodRecLength;  //接收数据长度//from RxEvent isr
/* Private function prototypes -----------------------------------------------*/
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Receive byte from sender
  * @param  c: Character
  * @param  timeout: Timeout
  * @retval 0: Byte received
  *         -1: Timeout
  */
static int32_t Receive_Byte(uint8_t *c, uint16_t length, uint32_t timeout)
{
  HAL_UARTEx_ReceiveToIdle_DMA(&huart1, c, length);

  if(pdTRUE == xQueueReceive(Q_YmodemReclength,&s_u16_YmodRecLength,timeout))
  {//from RxEvent isr
    return 0;
  }
  return -1;
}

/**
  * @brief  Send a byte
  * @param  c: Character
  * @retval 0: Byte sent
  */
static uint32_t Send_Byte (uint8_t c)
{
  //todo：后续添加
  //SerialPutChar(c);
  HAL_UART_Transmit(&huart1,&c,1,1000);
  return 0;
}

/**
  * @brief  Receive a packet from sender
  * @param  data
  * @param  length
  * @param  timeout
  *     0: end of transmission
  *    -1: abort by sender
  *    >0: packet length
  * @retval 0: normally return
  *        -1: timeout or packet error
  *         1: abort by user
  */
static int32_t Receive_Packet (uint8_t *data, int32_t *length, uint32_t timeout)
{
  uint16_t packet_size;
  *length = 0;
  if (Receive_Byte(data,1030,timeout) != 0)
  {
    return -1;
  }

  switch (*data)
  {
    case SOH:
      packet_size = PACKET_SIZE;
      break;
    case STX:
      packet_size = PACKET_1K_SIZE;
      break;
    case EOT:
      return 0;
    case CA:
      if ((Receive_Byte(data, 1,timeout) == 0) && (*data == CA))
      {
        *length = -1;
        return 0;
      }
      else
      {
        return -1;
      }
    case ABORT1:
    case ABORT2:
      return 1;
    default:
      return -1;
  }
  if (data[PACKET_SEQNO_INDEX] != ((data[PACKET_SEQNO_COMP_INDEX] ^ 0xff) & 0xff))
  {
    return -1;
  }

  //校验搬运长度和接收长度是否一致
  if(s_u16_YmodRecLength  !=  (packet_size + PACKET_OVERHEAD))
  {
    return -1;
  }

  *length = packet_size;
  return 0;
}

/**
  * @brief  Receive a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
uint8_t file_size[FILE_SIZE_LENGTH], *file_ptr;
int32_t i, packet_length, session_done, file_done, packets_received, errors, session_begin, size = 0;
int32_t Ymodem_Receive (uint8_t *buf1,uint8_t *buf2)
{
  uint8_t *packet_data;
  packet_data = buf1;
  for (session_done = 0, errors = 0, session_begin = 0; ;)//初始化变量，进入循环
  {
    for (packets_received = 0, file_done = 0; ;)
    {
      switch (Receive_Packet(packet_data, &packet_length, 2000))
      {
        case 0:
          errors = 0;
          switch (packet_length)
          {
            /* Abort by sender */
            case - 1:
              Send_Byte(ACK);
              return 0;
            /* End of transmission */
            case 0:
              Send_Byte(ACK);
              file_done = 1;
              break;
            /* Normal packet */
            default:
              if ((packet_data[PACKET_SEQNO_INDEX] & 0xff) != (packets_received & 0xff))
              {
                Send_Byte(NAK);
              }
              else
              {
                if (packets_received == 0)
                {
                  /* Filename packet */
                  if (packet_data[PACKET_HEADER] != 0)
                  {
                    /* Filename packet has valid data */
                    for (i = 0, file_ptr = packet_data + PACKET_HEADER; (*file_ptr != 0) && (i < FILE_NAME_LENGTH);)
                    {
                      file_name[i++] = *file_ptr++;
                    }
                    file_name[i++] = '\0';
                    //获取文件大小
                    for (i = 0, file_ptr ++; (*file_ptr != ' ') && (i < FILE_SIZE_LENGTH);)
                    {
                      file_size[i++] = *file_ptr++;
                    }
                    file_size[i++] = '\0';
                    Str2Int(file_size, &size);
                    //size = (uint32_t)atoi((char *)file_size);
                    //需要擦除某些W25q的扇区，根据size的大小决定
                    //通知，不做任何处理
                    int32_t * pu32_size = &size;///这里传的地址之后还要取地址？二级指针？
                    xQueueSend(Queue_AppDataBuffer,&pu32_size,0);//队列是指针 W25q的扇区，根据size的大小决定
                      if(packet_data == buf1)//切换buf
                    {
                      packet_data = buf2;
                    }
                    else
                    {
                      packet_data = buf1;
                    }
                    Send_Byte(ACK);
                    Send_Byte(CRC16);
                  }
                  /* Filename packet is empty, end session */
                  else
                  {
                    Send_Byte(ACK);
                    file_done = 1;
                    session_done = 1;
                    break;
                  }
                }
                /* Data packet */
                else
                {///这一段没懂
                  packet_data += PACKET_HEADER;//跳过头信息
                  xQueueSend(Queue_AppDataBuffer,&packet_data,0);//// 将数据指针发送到队列
									packet_data -= PACKET_HEADER;//恢复帧头地址
									
                  xSemaphoreTake(Semaphore_ExtFlashState,portMAX_DELAY);//这里只是为了获取防止 其他任务在运行 使用数据
                  xSemaphoreGive(Semaphore_ExtFlashState);

                  if(packet_data == buf1)//切换buf
                  {
                    packet_data = buf2;
                  }
                  else
                  {
                    packet_data = buf1;
                  }
                  Send_Byte(ACK);
                }
                packets_received ++;
                session_begin = 1;
              }
          }
          break;
        case 1:
          Send_Byte(CA);
          Send_Byte(CA);
          return -3;
        default:
        //发送的时候需要错误超过次数退出
         if (session_begin > 0)
         {
            errors ++;
         }
          if (errors > MAX_ERRORS)
          {
            Send_Byte(CA);
            Send_Byte(CA);
            return 0;
          }
          Send_Byte(CRC16);
          break;
      }
      if (file_done != 0)
      {
        break;
      }
    }
    if (session_done != 0)
    {
      break;
    }
  }
  return (int32_t)size;
}

/**
  * @brief  check response using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
int32_t Ymodem_CheckResponse(uint8_t c)
{
  return 0;
}

/**
  * @brief  Prepare the first block
  * @param  timeout
  *     0: end of transmission
  */
void Ymodem_PrepareIntialPacket(uint8_t *data, const uint8_t* fileName, uint32_t *length)
{
  uint16_t i, j;
  uint8_t file_ptr[10];
  
  /* Make first three packet */
  data[0] = SOH;
  data[1] = 0x00;
  data[2] = 0xff;
  
  /* Filename packet has valid data */
  for (i = 0; (fileName[i] != '\0') && (i < FILE_NAME_LENGTH);i++)
  {
     data[i + PACKET_HEADER] = fileName[i];
  }

  data[i + PACKET_HEADER] = 0x00;
  //todo:后续添加
  //Int2Str (file_ptr, *length);
  for (j =0, i = i + PACKET_HEADER + 1; file_ptr[j] != '\0' ; )
  {
     data[i++] = file_ptr[j++];
  }
  
  for (j = i; j < PACKET_SIZE + PACKET_HEADER; j++)
  {
    data[j] = 0;
  }
}

/**
  * @brief  Prepare the data packet
  * @param  timeout
  *     0: end of transmission
  */
void Ymodem_PreparePacket(uint8_t *SourceBuf, uint8_t *data, uint8_t pktNo, uint32_t sizeBlk)
{
  uint16_t i, size, packetSize;
  uint8_t* file_ptr;
  
  /* Make first three packet */
  packetSize = sizeBlk >= PACKET_1K_SIZE ? PACKET_1K_SIZE : PACKET_SIZE;
  size = sizeBlk < packetSize ? sizeBlk :packetSize;
  if (packetSize == PACKET_1K_SIZE)
  {
     data[0] = STX;
  }
  else
  {
     data[0] = SOH;
  }
  data[1] = pktNo;
  data[2] = (~pktNo);
  file_ptr = SourceBuf;
  
  /* Filename packet has valid data */
  for (i = PACKET_HEADER; i < size + PACKET_HEADER;i++)
  {
     data[i] = *file_ptr++;
  }
  if ( size  <= packetSize)
  {
    for (i = size + PACKET_HEADER; i < packetSize + PACKET_HEADER; i++)
    {
      data[i] = 0x1A; /* EOF (0x1A) or 0x00 */
    }
  }
}

/**
  * @brief  Update CRC16 for input byte
  * @param  CRC input value 
  * @param  input byte
   * @retval None
  */
uint16_t UpdateCRC16(uint16_t crcIn, uint8_t byte)
{
 uint32_t crc = crcIn;
 uint32_t in = byte|0x100;
 do
 {
 crc <<= 1;
 in <<= 1;
 if(in&0x100)
 ++crc;
 if(crc&0x10000)
 crc ^= 0x1021;
 }
 while(!(in&0x10000));
 return crc&0xffffu;
}


/**
  * @brief  Cal CRC16 for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
uint16_t Cal_CRC16(const uint8_t* data, uint32_t size)
{
 uint32_t crc = 0;
 const uint8_t* dataEnd = data+size;
 while(data<dataEnd)
  crc = UpdateCRC16(crc,*data++);
 
 crc = UpdateCRC16(crc,0);
 crc = UpdateCRC16(crc,0);
 return crc&0xffffu;
}

/**
  * @brief  Cal Check sum for YModem Packet
  * @param  data
  * @param  length
   * @retval None
  */
uint8_t CalChecksum(const uint8_t* data, uint32_t size)
{
 uint32_t sum = 0;
 const uint8_t* dataEnd = data+size;
 while(data < dataEnd )
   sum += *data++;
 return sum&0xffu;
}

/**
  * @brief  Transmit a data packet using the ymodem protocol
  * @param  data
  * @param  length
   * @retval None
  */
void Ymodem_SendPacket(uint8_t *data, uint16_t length)
{
  uint16_t i;
  i = 0;
  while (i < length)
  {
    Send_Byte(data[i]);
    i++;
  }
}

/**
  * @brief  Transmit a file using the ymodem protocol
  * @param  buf: Address of the first byte
  * @retval The size of the file
  */
uint8_t Ymodem_Transmit (uint8_t *buf, const uint8_t* sendFileName, uint32_t sizeFile)
{
  
  uint8_t packet_data[PACKET_1K_SIZE + PACKET_OVERHEAD];
  uint8_t FileName[FILE_NAME_LENGTH];
  uint8_t *buf_ptr, tempCheckSum ;
  uint16_t tempCRC, blkNumber;
  uint8_t receivedC[2], CRC16_F = 0, i;
  uint32_t errors, ackReceived, size = 0, pktSize;

  errors = 0;
  ackReceived = 0;
  for (i = 0; i < (FILE_NAME_LENGTH - 1); i++)
  {
    FileName[i] = sendFileName[i];
  }
  CRC16_F = 1;       
    
  /* Prepare first block */
  Ymodem_PrepareIntialPacket(&packet_data[0], FileName, &sizeFile);
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
    /* Send CRC or Check Sum based on CRC16_F */
    if (CRC16_F)
    {
       tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCRC >> 8);
       Send_Byte(tempCRC & 0xFF);
    }
    else
    {
       tempCheckSum = CalChecksum (&packet_data[3], PACKET_SIZE);
       Send_Byte(tempCheckSum);
    }
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 1,10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* Packet transfered correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
  }while (!ackReceived && (errors < 0x0A));
  
  if (errors >=  0x0A)
  {
    return errors;
  }
  buf_ptr = buf;
  size = sizeFile;
  blkNumber = 0x01;
  /* Here 1024 bytes package is used to send the packets */
  
  
  /* Resend packet if NAK  for a count of 10 else end of commuincation */
  while (size)
  {
    /* Prepare next packet */
    Ymodem_PreparePacket(buf_ptr, &packet_data[0], blkNumber, size);
    ackReceived = 0;
    receivedC[0]= 0;
    errors = 0;
    do
    {
      /* Send next packet */
      if (size >= PACKET_1K_SIZE)
      {
        pktSize = PACKET_1K_SIZE;
       
      }
      else
      {
        pktSize = PACKET_SIZE;
      }
      Ymodem_SendPacket(packet_data, pktSize + PACKET_HEADER);
      /* Send CRC or Check Sum based on CRC16_F */
      /* Send CRC or Check Sum based on CRC16_F */
      if (CRC16_F)
      {
         tempCRC = Cal_CRC16(&packet_data[3], pktSize);
         Send_Byte(tempCRC >> 8);
         Send_Byte(tempCRC & 0xFF);
      }
      else
      {
        tempCheckSum = CalChecksum (&packet_data[3], pktSize);
        Send_Byte(tempCheckSum);
      }
      
      /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0],1, 100000) == 0)  && (receivedC[0] == ACK))
      {
        ackReceived = 1;  
        if (size > pktSize)
        {
           buf_ptr += pktSize;  
           size -= pktSize;
                            //本身App的大小限制在0x10000
           if (blkNumber == (0x10000/0x400))
           {
             return 0xFF; /*  error */
           }
           else
           {
              blkNumber++;
           }
        }
        else
        {
          buf_ptr += pktSize;
          size = 0;
        }
      }
      else
      {
        errors++;
      }
    }while(!ackReceived && (errors < 0x0A));
    /* Resend packet if NAK  for a count of 10 else end of commuincation */
    
    if (errors >=  0x0A)
    {
      return errors;
    }
    
  }
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;
  do 
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0], 1, 10000) == 0)  && receivedC[0] == ACK)
      {
        ackReceived = 1;  
      }
      else
      {
        errors++;
      }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  
  /* Last packet preparation */
  ackReceived = 0;
  receivedC[0] = 0x00;
  errors = 0;

  packet_data[0] = SOH;
  packet_data[1] = 0;
  packet_data [2] = 0xFF;

  for (i = PACKET_HEADER; i < (PACKET_SIZE + PACKET_HEADER); i++)
  {
     packet_data [i] = 0x00;
  }
  
  do 
  {
    /* Send Packet */
    Ymodem_SendPacket(packet_data, PACKET_SIZE + PACKET_HEADER);
    /* Send CRC or Check Sum based on CRC16_F */
    tempCRC = Cal_CRC16(&packet_data[3], PACKET_SIZE);
    Send_Byte(tempCRC >> 8);
    Send_Byte(tempCRC & 0xFF);
  
    /* Wait for Ack and 'C' */
    if (Receive_Byte(&receivedC[0], 1, 10000) == 0)  
    {
      if (receivedC[0] == ACK)
      { 
        /* Packet transfered correctly */
        ackReceived = 1;
      }
    }
    else
    {
        errors++;
    }
 
  }while (!ackReceived && (errors < 0x0A));
  /* Resend packet if NAK  for a count of 10  else end of commuincation */
  if (errors >=  0x0A)
  {
    return errors;
  }  
  
  do 
  {
    Send_Byte(EOT);
    /* Send (EOT); */
    /* Wait for Ack */
      if ((Receive_Byte(&receivedC[0], 1, 10000) == 0)  && receivedC[0] == ACK)
      {
        ackReceived = 1;  
      }
      else
      {
        errors++;
      }
  }while (!ackReceived && (errors < 0x0A));
    
  if (errors >=  0x0A)
  {
    return errors;
  }
  return 0; /* file trasmitted successfully */
}
