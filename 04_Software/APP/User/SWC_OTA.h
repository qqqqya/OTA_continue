#ifndef __SWC_OTA_H
#define __SWC_OTA_H

#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "main.h"
#include "cmsis_os.h"
void ota_task_runnable(void *argument);

#endif 

