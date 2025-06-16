/*
 * timer.c
 *
 *  Created on: Jun 13, 2025
 *      Author: JasonHong
 */
#include "timer.h"

QueueHandle_t xTimerQueue;
SemaphoreHandle_t xTimerMutex;
RTC_TimeTypeDef sTime_Init;
RTC_DateTypeDef sDate_Init;
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;
UART_HandleTypeDef* timer_huart;
extern QueueHandle_t xESP32Queue;
extern RTC_HandleTypeDef hrtc;


void Timer_Init(UART_HandleTypeDef* huart)
{
	timer_huart = huart;
}

void Timer_OS_Resources_Init()
{
	xTimerQueue = xQueueCreate(4, sizeof(TimerMsgStruct));
	xTimerMutex = xSemaphoreCreateMutex();

	xSemaphoreTake(xTimerMutex, portMAX_DELAY);
	HAL_RTC_GetTime(&hrtc, &sTime_Init, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate_Init, RTC_FORMAT_BIN);
	xSemaphoreGive(xTimerMutex);
}

void SyncTimeEventSender(void)
{
	ESP32MsgStruct eps32Msg = {0};
	strcpy(eps32Msg.msg, "GET_TIME\n\r");
	xQueueSend(xESP32Queue, &eps32Msg, pdMS_TO_TICKS(100));
}

void SyncTimeHandler(void)
{
    // "YYYY/MM/DD HH:MM:SS"
	TimerMsgStruct timerMsg;
	while(1){

		if (xQueueReceive(xTimerQueue, &timerMsg, pdMS_TO_TICKS(5000)) == pdPASS)
		{
			xSemaphoreTake(xTimerMutex, portMAX_DELAY);
			char tm_buf[20];
			size_t len = strcspn(timerMsg.msg, "\r\n\0");
			if (len >= sizeof(tm_buf)) len = sizeof(tm_buf) - 1;
			memcpy(tm_buf, timerMsg.msg, len);
			tm_buf[len] = '\0';

			HAL_SuspendTick();	
    		__HAL_RTC_WRITEPROTECTION_DISABLE(&hrtc);

			uint32_t year, mon, day, hh, mm, ss;
			if (sscanf(tm_buf, "%d/%d/%d %d:%d:%d",
					   &year, &mon, &day, &hh, &mm, &ss) == 6)
			{
				RTC_DateTypeDef sCurDate = { .Year = year % 100, .Month = mon, .Date = day, .WeekDay = 0 };
				RTC_TimeTypeDef sCurTime = { .Hours = hh, .Minutes = mm, .Seconds = ss, .TimeFormat = RTC_HOURFORMAT12_AM };
				HAL_RTC_SetDate(&hrtc, &sCurDate, RTC_FORMAT_BIN);
				HAL_RTC_SetTime(&hrtc, &sCurTime, RTC_FORMAT_BIN);
				HAL_RTC_GetTime(&hrtc, &sTime_Init, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &sDate_Init, RTC_FORMAT_BIN);
				HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
				HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
				SendMsg(&timer_huart, "SyncTime: RTC set to %04d/%02d/%02d %02d:%02d:%02d\r\n",
						2000+year, mon, day, hh, mm, ss);
			}
			else
			{
				SendMsg(&timer_huart, "SyncTime: parse failed: '%s'\r\n", timerMsg.msg);
			}

			__HAL_RTC_WRITEPROTECTION_ENABLE(&hrtc);
    		HAL_ResumeTick();
			xSemaphoreGive(xTimerMutex);
		}
		else
		{
			SendMsg(&timer_huart, "SyncTime: timeout waiting for GET_TIME\r\n");
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void getCurrentTime(char* time, int size)
{
	xSemaphoreTake(xTimerMutex, portMAX_DELAY);
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	snprintf(time, size, "%02d/%02d %02d:%02d:%02d",
				sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds);
	time[size-1] = '\0';
	xSemaphoreGive(xTimerMutex);
}

void getTimeSinceStart(char* time, int size)
{
	xSemaphoreTake(xTimerMutex, portMAX_DELAY);
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	uint8_t Seconds = sTime.Seconds - sTime_Init.Seconds >= 0
			? sTime.Seconds - sTime_Init.Seconds
			: sTime.Seconds + (60 - sTime_Init.Seconds);
	uint8_t Minutes = sTime.Minutes - sTime_Init.Minutes >= 0
			? sTime.Minutes - sTime_Init.Minutes
			: sTime.Minutes + (60 - sTime_Init.Minutes);
	Minutes -= sTime.Seconds - sTime_Init.Seconds >= 0 ? 0 : 1;
	uint8_t Hours = sTime.Hours - sTime_Init.Hours >= 0
				? sTime.Hours - sTime_Init.Hours
				: sTime.Hours + (24 - sTime_Init.Hours);
	Hours -= sTime.Minutes - sTime_Init.Minutes >= 0 ? 0 : 1;

	snprintf(time, size, "[%02d:%02d:%02d]", Hours, Minutes, Seconds);
	time[size-1] = '\0';
	xSemaphoreGive(xTimerMutex);
}
