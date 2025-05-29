/*
 * esp32.h
 *
 *  Created on: May 17, 2025
 *      Author: JasonHong
 */

#ifndef INC_ESP32_H_
#define INC_ESP32_H_
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include "semphr.h"


typedef struct
{
	char msg[100];
} ESP32MsgStruct;

void ESP32_Init(UART_HandleTypeDef* eps32_huart, UART_HandleTypeDef* log_huart);

void ESP32Sender(void *pvParameters);

void ESP32Receiver(void *pvParameters);

#endif /* INC_ESP32_H_ */
