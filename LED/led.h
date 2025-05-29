/*
 * led.h
 *
 *  Created on: May 18, 2025
 *      Author: JasonHong
 */

#ifndef LED_H_
#define LED_H_

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "queue.h"
#include "semphr.h"

typedef struct
{
	int r;
	int g;
	int b;
} LEDMsgStruct;

void LED_Init();

void LED_OS_Resources_Init();

void LEDHandler();

void LEDTask();


#endif /* LED_H_ */
