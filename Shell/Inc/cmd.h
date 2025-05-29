

#ifndef SRC_CMD_H_
#define SRC_CMD_H_
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "queue.h"
#include "task.h"

typedef uint8_t (CmdFunction)(uint8_t argc, char **argv);

typedef struct
{
	char *name;
	char *help;
	CmdFunction *func;
} CmdStruct;

void command_Init(UART_HandleTypeDef* huart);

bool CommandExecute(uint8_t argc, char **argv);

#endif /* SRC_CMD_H_ */



