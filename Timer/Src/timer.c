/*
 * timer.c
 *
 *  Created on: Jun 13, 2025
 *      Author: JasonHong
 */
#include "timer.h"

RTC_TimeTypeDef sTime_Init;
RTC_DateTypeDef sDate_Init;
RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;
UART_HandleTypeDef* timer_huart;
extern RTC_HandleTypeDef hrtc;


void Timer_Init(UART_HandleTypeDef* huart)
{
	timer_huart = huart;
	HAL_RTC_GetTime(&hrtc, &sTime_Init, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate_Init, RTC_FORMAT_BIN);
}

void getCurrentTime(char* time, int size)
{
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	snprintf(time, size, "%02d/%02d %02d:%02d:%02d",
				sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds);
	time[size-1] = '\0';
}

void getTimeSinceStart(char* time, int size)
{
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
}
