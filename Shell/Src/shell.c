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
QueueHandle_t xShellQueue;
char readBuffer[READBUF_LEN] = {0};

void Shell_Init(UART_HandleTypeDef *huart)
{
	shell_huart = huart;
	ReadLine_Init(huart);
	command_Init(huart);
}

void Shell_OS_Resources_Init()
{
	xShellQueue = xQueueCreate(4, sizeof(ShellMsgStruct));

	const char *welcome_msg = "\r\nSTM32 Shell Initialized. Type 'help' for commands.\r\n";
	SendMsg(shell_huart, welcome_msg);
}

void ShellHandler(void *pvParameters)
{
	static uint8_t argc = 0;
	static char *argv[MAX_ARGS] = {0};

	while (1)
	{
		if (ReadLine(readBuffer, READBUF_LEN))
		{
			memset(&argc, 0, sizeof(argc));
			memset(argv, 0, sizeof(argv));

			if (ArgAnalyze(readBuffer, &argc, argv))
			{
				CommandExecute(argc, argv);
				SetLCDCommandStatus(argv[0]);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

bool TryGetCommand(ShellMsgStruct* pShellMsg, char* cmd)
{	
	char buf[128];
	strncpy(buf, pShellMsg->msg, sizeof(buf)-1);
	buf[sizeof(buf)-1] = '\0';

	char *token = strtok(buf, " ");
	if (token != NULL) {
		int len = strlen(token);
		strncpy(cmd, token, len + 1);
        cmd[len] = '\0';
		return true;
	}
	return false;
}

void ParserCommand(ShellMsgStruct* pShellMsg, char* cmd)
{
	if(strcmp(cmd, "led") == 0)
	{
		int r, g, b;
		if (sscanf(pShellMsg->msg,
					"%15s {\"r\":%d,\"g\":%d,\"b\":%d}",
					cmd, &r, &g, &b) == 4)
		{
			snprintf(pShellMsg->msg, sizeof(pShellMsg->msg), "%s %d %d %d", cmd, r, g, b);
		}
	}
}

void CommandReceiver(void *pvParameters)
{
	char cmd[40];
	static uint8_t argc = 0;
	static char *argv[MAX_ARGS] = {0};
	ShellMsgStruct shellMsg;
	while(1)
	{
		if (xQueueReceive(xShellQueue, &shellMsg, portMAX_DELAY) == pdPASS) {

			if(TryGetCommand(&shellMsg, cmd))
			{
				SendMsg(shell_huart, "Command : %s\r\n", cmd);
				ParserCommand(&shellMsg, cmd);
			}

			SendMsg(shell_huart, "ParserCommand : %s\r\n", shellMsg.msg);


			memset(&argc, 0, sizeof(argc));
			memset(argv, 0, sizeof(argv));
			if (ArgAnalyze(shellMsg.msg, &argc, argv))
			{
				CommandExecute(argc, argv);
				SetLCDCommandStatus(argv[0]);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}
