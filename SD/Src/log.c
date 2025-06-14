/*
 * log.c
 *
 *  Created on: Jun 13, 2025
 *      Author: JasonHong
 */

#include "log.h"

UART_HandleTypeDef* log_huart;
QueueHandle_t xLogQueue;
SemaphoreHandle_t xLogMutex;

void Log_Init(UART_HandleTypeDef* huart)
{
	log_huart = huart;
}

void Log_OS_Resources_Init()
{
	xLogQueue = xQueueCreate(4, sizeof(LogMsgStruct));
	xLogMutex = xSemaphoreCreateMutex();

	// creat and clear
	FIL logFile;
	FRESULT fr;
	xSemaphoreTake(xLogMutex, portMAX_DELAY);
	fr = f_open(&logFile, "log.txt", FA_WRITE | FA_CREATE_ALWAYS);
	if (fr == FR_OK) {
		f_close(&logFile);
		SendMsg(log_huart, "Log file cleared.\r\n");
	} else {
		SendMsg(log_huart, "Failed to clear log file, error = %d\r\n", fr);
	}
	xSemaphoreGive(xLogMutex);
}

void LogWriter(const char *format, ...)
{
  char* buf[100];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, sizeof(buf), format, args);
  va_end(args);

  char time[20];
  getTimeSinceStart(time, sizeof(time));

  LogMsgStruct logMsg;
  snprintf(logMsg.msg, sizeof(logMsg.msg), "%s  %s\n\r", time, buf);
  if (xQueueSend(xLogQueue, &logMsg, pdMS_TO_TICKS(100)) != pdPASS)
  {
	  SendMsg(log_huart, "Failed to send log message!\r\n");
  }
}

 void PrintLogFile(void) {
	FIL logFile;
	FRESULT fr;
	UINT br;
	char buffer[128];
	xSemaphoreTake(xLogMutex, portMAX_DELAY);
	fr = f_open(&logFile, "log.txt", FA_READ);
	if (fr != FR_OK) {
		SendMsg(log_huart, "Failed to open log.txt (Error: %d)\r\n", fr);
		xSemaphoreGive(xLogMutex);
		return;
	}
	SendMsg(log_huart, "\r\nLog file contents:\r\n");
	do {
		fr = f_read(&logFile, buffer, sizeof(buffer) - 1, &br);
		if (fr != FR_OK) {
			SendMsg(log_huart, "Error reading log.txt (Error: %d)\r\n", fr);
			break;
		}
		buffer[br] = '\0';
		SendMsg(log_huart, "%s", buffer);
	} while (br == sizeof(buffer) - 1);
	f_close(&logFile);
	xSemaphoreGive(xLogMutex);
 }

 void LogHandler(void *pvParameters) {
	FIL logFile;
	FRESULT fr;
	UINT bw;
	LogMsgStruct logMsg;
	bool fileOpened = false;

	while(1){
		if (xQueueReceive(xLogQueue, &logMsg, portMAX_DELAY) == pdPASS) {
			xSemaphoreTake(xLogMutex, portMAX_DELAY);
			if(!fileOpened) {
				fr = f_open(&logFile, "log.txt", FA_OPEN_APPEND | FA_WRITE);
				if (fr != FR_OK){
					SendMsg(log_huart, "LogTask: Failed to open log.txt, error = %d\r\n", fr);
					xSemaphoreGive(xLogMutex);
					continue;
				}
				fileOpened = true;
			}

			fr = f_write(&logFile, logMsg.msg, strlen(logMsg.msg), &bw);
			if (fr == FR_OK){
				f_sync(&logFile);
				SendMsg(log_huart, "LogTask: Wrote log entry.\r\n");
				f_close(&logFile);
				fileOpened = false;
				xSemaphoreGive(xLogMutex);
				PrintLogFile();
			} else {
				SendMsg(log_huart, "LogTask: f_write error: %d\r\n", fr);
				xSemaphoreGive(xLogMutex);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
 }
