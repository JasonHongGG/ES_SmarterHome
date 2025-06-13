#ifndef __SHELL_H
#define __SHELL_H
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"



typedef struct
{
	char msg[200];
} ShellMsgStruct;

void Shell_Init(UART_HandleTypeDef* huart);

void ShellHandler(void *pvParameters);

void CommandReceiver(void *pvParameters);

#endif
