/*
 * timer.h
 *
 *  Created on: Jun 13, 2025
 *      Author: JasonHong
 */

#ifndef INC_TIMER_H_
#define INC_TIMER_H_

#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "queue.h"
#include "task.h"
#include "msgHandler.h"
#include "esp32.h"

typedef struct
{
	char msg[100];
} TimerMsgStruct;

void Timer_Init();

void Timer_OS_Resources_Init();

void SyncTimeEventSender(void);

void SyncTimeHandler(void);

void getCurrentTime(char* time, int size);

void getTimeSinceStart(char* time, int size);

#endif /* INC_TIMER_H_ */
