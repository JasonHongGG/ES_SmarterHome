/*
 * log.h
 *
 *  Created on: Jun 13, 2025
 *      Author: JasonHong
 */

#ifndef INC_LOG_H_
#define INC_LOG_H_

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"
#include "timer.h"
#include "fatfs.h"
#include "msgHandler.h"

typedef struct
{
	char msg[100];
} LogMsgStruct;

void Log_Init(UART_HandleTypeDef* huart);

void Log_OS_Resources_Init();

void PrintLogFile();

void LogWriter(const char *format, ...);

void LogHandler(void *pvParameters);

#endif /* INC_LOG_H_ */
