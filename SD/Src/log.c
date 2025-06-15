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
extern QueueHandle_t xESP32Queue; 

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

void PrintLogFile(void) 
{
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

void LogHandler(void *pvParameters) 
{
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
//				SendMsg(log_huart, "LogTask: Wrote log entry.\r\n");
				f_close(&logFile);
				fileOpened = false;
				xSemaphoreGive(xLogMutex);
			} else {
				SendMsg(log_huart, "LogTask: f_write error: %d\r\n", fr);
				xSemaphoreGive(xLogMutex);
			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void UploadLogFile(void) 
{
	FIL logFile;
	FRESULT fr;
	UINT br;
	char buffer[100];

	xSemaphoreTake(xLogMutex, portMAX_DELAY);

	fr = f_open(&logFile, "log.txt", FA_READ);
	if (fr != FR_OK) {
		SendMsg(log_huart, "UploadLogFile: open failed %d\r\n", fr);
		if (xLogMutex) xSemaphoreGive(xLogMutex);
		return;
	}

	ESP32MsgStruct startEventMsg;
	snprintf(startEventMsg.msg, sizeof(startEventMsg.msg)-1, "LOG_UPLOAD_START\n\r");
	startEventMsg.msg[sizeof(startEventMsg.msg)-1] = '\0';
	if (xQueueSend(xESP32Queue, &startEventMsg, pdMS_TO_TICKS(100)) != pdPASS) {
		SendMsg(log_huart, "UploadLogFile: queue full\r\n");
	}

	while (1) {
		fr = f_read(&logFile, buffer, sizeof(buffer)-1, &br);
		if (fr != FR_OK || br == 0) break;
		buffer[br] = '\0';

		ESP32MsgStruct msg;
		strncpy(msg.msg, buffer, sizeof(msg.msg)-1);
		msg.msg[sizeof(msg.msg)-1] = '\0';
		SendMsg(log_huart, "\r\n%s\r\n", msg.msg);

		if (xQueueSend(xESP32Queue, &msg, pdMS_TO_TICKS(100)) != pdPASS) {
			SendMsg(log_huart, "UploadLogFile: queue full\r\n");
			break;
		}
		vTaskDelay(pdMS_TO_TICKS(10)); // 給 ESP32Sender 處理時間
	}

	ESP32MsgStruct endEventMsg;
	snprintf(endEventMsg.msg, sizeof(endEventMsg.msg)-1, "LOG_UPLOAD_END\n\r");
	endEventMsg.msg[sizeof(endEventMsg.msg)-1] = '\0';
	if (xQueueSend(xESP32Queue, &endEventMsg, pdMS_TO_TICKS(100)) != pdPASS) {
		SendMsg(log_huart, "UploadLogFile: queue full\r\n");
	}

	f_close(&logFile);
	SendMsg(log_huart, "\r\nUploadLogFile: done\r\n");

	xSemaphoreGive(xLogMutex);
}
