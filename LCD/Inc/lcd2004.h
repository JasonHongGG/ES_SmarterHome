/*
 * lcd2004.h
 *
 *  Created on: May 17, 2025
 *      Author: JasonHong
 */

#ifndef INC_LCD2004_H_
#define INC_LCD2004_H_
#include "i2c_lcd.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

typedef struct
{
	uint8_t row;
	uint8_t col;
	char msg[32];
} LCDMsgStruct;

void LCD2004_Init(I2C_HandleTypeDef *hi2c, uint8_t address, UART_HandleTypeDef* haurt);

void LCD2004_OS_Resources_Init();

void SetLCDCommandStatus(char* str);

void LCDHandler(void *pvParameters);

#endif /* INC_LCD2004_H_ */
