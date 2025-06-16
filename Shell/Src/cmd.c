#include "cmd.h"
#include "msgHandler.h"
#include "lcd2004.h"
#include "esp32.h"
#include "led.h"
#include "sd.h"
#include "log.h"
#include "timer.h"
extern UART_HandleTypeDef* shell_huart;
extern QueueHandle_t xLCDQueue;
extern QueueHandle_t xESP32Queue;
extern QueueHandle_t xLEDQueue;
extern QueueHandle_t xSDQueue;
extern QueueHandle_t xLogQueue;
extern QueueHandle_t xTimerQueue;

static void CommandPrint(uint8_t argc, char **argv);
static uint8_t PrintArgs(uint8_t argc, char **argv);
void LCDShowMsg(uint8_t argc, char **argv);
void ESP32SendMsg(uint8_t argc, char **argv);
void LEDChangeColor(uint8_t argc, char **argv);
void ParseStorage(uint8_t argc, char **argv);
void WriteLog(uint8_t argc, char **argv);
void PrintLog(uint8_t argc, char **argv);
void UploadLog(uint8_t argc, char **argv);
void UpdateTimer(uint8_t argc, char **argv);
void SynchronizeTimer(uint8_t argc, char **argv);
void RelaySwitch(uint8_t argc, char **argv);

static const CmdStruct CommandList[] =
{
	{"help", "Show this help message", "help", CommandPrint},
	{"args", "Show all args", "args", PrintArgs},
	{"lcd", "Show message on LCD2004", "lcd row column message", LCDShowMsg},
	{"esp32", "Send message to ESP32", "esp32 message", ESP32SendMsg},
	{"led", "Change LED color", "led red green blue",  LEDChangeColor},
	{"sd", "Parse file from SD", "sd fileName", ParseStorage},
	{"log", "Write log", "log message",  WriteLog},
	{"logPrint", "Write log", "logPrint", PrintLog},
	{"logUpload", "Upload log", "logUpload", UploadLog},
	{"updateTimer", "Update Timer", "Update YYYY/MM/DD HH:MM:SS", UpdateTimer},
	{"syncTime", "Synchronize timer", "syncTime", SynchronizeTimer},
	{"relay", "relay switch", "relay 0/1", RelaySwitch},
	{NULL, NULL, NULL},
};

void command_Init(UART_HandleTypeDef* huart)
{
	shell_huart = huart;
}

void RelaySwitch(uint8_t argc, char **argv)
{
	if(argc < 2) {
		SendMsg(shell_huart, "\r\RelaySwitch: Not enough arguments for this command.\r\n");
		return;
	}

	int open = (int)atoi(argv[1]);
	HAL_GPIO_WritePin(RelayController_GPIO_Port, RelayController_Pin, open);
}

void SynchronizeTimer(uint8_t argc, char **argv)
{
	SyncTimeEventSender();
}

void UpdateTimer(uint8_t argc, char **argv)
{
	if(argc < 3) {
		SendMsg(shell_huart, "\r\nUpdateTimer: Not enough arguments for this command.\r\n");
		return;
	}

	TimerMsgStruct timerMsg;
	snprintf(timerMsg.msg, sizeof(timerMsg.msg), "%s %s", argv[1], argv[2]);
	timerMsg.msg[sizeof(timerMsg.msg)-1] = '\0';

	if (xQueueSend(xTimerQueue, &timerMsg, 0) != pdPASS) {
		SendMsg(shell_huart, "\r\nUpdateTimer: Queue full or error.\r\n");
	}
}

void UploadLog(uint8_t argc, char **argv)
{
	UploadLogFile();
}

void PrintLog(uint8_t argc, char **argv)
{
	PrintLogFile();
}

void WriteLog(uint8_t argc, char **argv)
{
	if(argc < 2) {
		SendMsg(shell_huart, "\r\WriteLog: Not enough arguments for this command.\r\n");
		return;
	}
	LogWriter(argv[1]);
}

void ParseStorage(uint8_t argc, char **argv)
{
	if(argc < 2) {
		SendMsg(shell_huart, "\r\nParseStorage: Not enough arguments for this command.\r\n");
		return;
	}

	LogWriter("ParseStorage");
	SDMsgStruct sdMsg;
	strncpy(sdMsg.msg, argv[1], sizeof(sdMsg.msg)-1);
	sdMsg.msg[sizeof(sdMsg.msg)-1] = '\0';

	if (xQueueSend(xSDQueue, &sdMsg, 0) != pdPASS) {
		SendMsg(shell_huart, "\r\ParseStorage: Queue full or error.\r\n");
	}
}

void LEDChangeColor(uint8_t argc, char **argv)
{
	if(argc < 4) {
		SendMsg(shell_huart, "\r\ESP32SendMsg: Not enough arguments for this command.\r\n");
		return;
	}

	LogWriter("LEDChangeColor");
	LEDMsgStruct LEDMsg;
	LEDMsg.r = (int)atoi(argv[1]);
	LEDMsg.g = (int)atoi(argv[2]);
	LEDMsg.b = (int)atoi(argv[3]);

	if (xQueueSend(xLEDQueue, &LEDMsg, 0) != pdPASS) {
		SendMsg(shell_huart, "\r\LEDMsg: Queue full or error.\r\n");
	}
}

void ESP32SendMsg(uint8_t argc, char **argv)
{
	if(argc < 2) {
		SendMsg(shell_huart, "\r\ESP32SendMsg: Not enough arguments for this command.\r\n");
		return;
	}

	LogWriter("ESP32SendMsg");
	SDMsgStruct esp32Msg;
	strncpy(esp32Msg.msg, argv[1], sizeof(esp32Msg.msg)-1);
	esp32Msg.msg[sizeof(esp32Msg.msg)-1] = '\0';

	if (xQueueSend(xESP32Queue, &esp32Msg, 0) != pdPASS) {
		SendMsg(shell_huart, "\r\ESP32SendMsg: Queue full or error.\r\n");
	}
}

void LCDShowMsg(uint8_t argc, char **argv)
{
	if(argc < 4) {
        SendMsg(shell_huart, "\r\nLCDShowMsg: Not enough arguments for this command.\r\n");
        return;
    }
	for(int j = 1; j <= 2; j++) {
		for (int i = 0; argv[1][i]; i++) {
			if (argv[1][i] < '0' || argv[1][i] > '9') {
				SendMsg(shell_huart, "\r\nLCDShowMsg: Row and Column must be a number.\r\n");
				return;
			}
		}
	}
	if ((uint8_t)atoi(argv[1]) > 3) {
        SendMsg(shell_huart, "\r\nLCDShowMsg: Row out of range (0-3).\r\n");
        return;
    }
    if ((uint8_t)atoi(argv[2]) > 19) {
        SendMsg(shell_huart, "\r\nLCDShowMsg: Column out of range (0-19).\r\n");
        return;
    }
	if (strlen(argv[3]) > (20 - (uint8_t)atoi(argv[2]))) {
        SendMsg(shell_huart, "\r\nLCDShowMsg: Message too long for this column.\r\n");
        return;
    }

	LogWriter("LCDShowMsg");
    LCDMsgStruct lcdMsg;
    lcdMsg.row = (uint8_t)atoi(argv[1]);
    lcdMsg.col = (uint8_t)atoi(argv[2]);
    strncpy(lcdMsg.msg, argv[3], sizeof(lcdMsg.msg)-1);
    lcdMsg.msg[sizeof(lcdMsg.msg)-1] = '\0';

    if (xQueueSend(xLCDQueue, &lcdMsg, 0) != pdPASS) {
        SendMsg(shell_huart, "\r\nLCDShowMsg: Queue full or error.\r\n");
    }
}

static void CommandPrint(uint8_t argc, char **argv)
{
	LogWriter("Help");
	SendMsg(shell_huart, "\r\n--------------------------------------------------------------------------------------\r\n");
	SendMsg(shell_huart, "    %12s|  %29s|  %29s\r\n", "NAME", "HELP", "USAGE");
	SendMsg(shell_huart, "--------------------------------------------------------------------------------------\r\n");
	for (uint8_t i=0; CommandList[i].name != NULL; i++)
	{
		SendMsg(shell_huart, "    %12s  %30s  %30s\r\n", CommandList[i].name, CommandList[i].help, CommandList[i].usage);
	}
	SendMsg(shell_huart, "--------------------------------------------------------------------------------------\r\n");
}


static uint8_t PrintArgs(uint8_t argc, char **argv)
{
	LogWriter("PrintArgs");
	SendMsg(shell_huart, "\r\n");
	for (uint8_t i=0; i<argc; i++)
	{
		SendMsg(shell_huart, "%d: %s \r\n", i, argv[i]);
	}
	return 0;
}

bool CommandExecute(uint8_t argc, char **argv)
{
	if (argc != 0 && argv != NULL)
	{
		for (uint8_t i=0; CommandList[i].name != NULL; i++)
		{
			if(strcmp(CommandList[i].name, argv[0]) == 0)
			{
				if(CommandList[i].func != NULL)
				{
					(CommandList[i].func)(argc, argv);
					return true;
				}
			}
		}
		SendMsg(shell_huart, "\r\nUnknown command : %s. Type 'help' for commands.\r\n", argv[0]);
	}
	return false;
}



