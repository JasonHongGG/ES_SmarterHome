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

#define PROMPT_LEN (0xFFU)
#define READBUF_LEN (1024U)
#define MAX_ARGS 10

void ReadLine_Init(UART_HandleTypeDef* huart);

uint32_t ReadLine();

bool ArgAnalyze(char* readBuffer, uint8_t *argc, char *argv[]);

#endif /* SRC_READLINE_H_ */
