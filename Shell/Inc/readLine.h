/*
 * readLine.h
 *
 *  Created on: May 14, 2025
 *      Author: JasonHong
 */

#ifndef SRC_READLINE_H_
#define SRC_READLINE_H_
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"

void ReadLine_Init(UART_HandleTypeDef* huart);

uint32_t ReadLine();

bool ArgAnalyze(uint8_t *argc, char *argv[]);

#endif /* SRC_READLINE_H_ */
