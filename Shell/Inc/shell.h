#ifndef __SHELL_H
#define __SHELL_H
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"

void Shell_Init(UART_HandleTypeDef* huart);

void ShellHandler(void *pvParameters);

#endif
