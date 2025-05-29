/*
 * shell.c
 *
 *  Created on: May 8, 2025
 *      Author: JasonHong
 */
#include "shell.h"
#include "cmd.h"
#include "readLine.h"
#include "msgHandler.h"
#define MAX_ARGS 10
UART_HandleTypeDef* shell_huart;

void Shell_Init(UART_HandleTypeDef *huart)
{
	shell_huart = huart;
	ReadLine_Init(huart);
	command_Init(huart);

	const char *welcome_msg = "\r\nSTM32 Shell Initialized. Type 'help' for commands.\r\n";
	SendMsg(shell_huart, welcome_msg);
}

void ShellHandler(void *pvParameters)
{
	static uint8_t argc = 0;
	static char *argv[MAX_ARGS] = {0};

	while (1)
	{
		if (ReadLine())
		{
			memset(&argc, 0, sizeof(argc));
			memset(argv, 0, sizeof(argv));

			if (ArgAnalyze(&argc, argv))
			{
				CommandExecute(argc, argv);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}
