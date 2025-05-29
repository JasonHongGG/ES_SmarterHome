/*
 * led.c
 *
 *  Created on: May 18, 2025
 *      Author: JasonHong
 */
#include "led.h"

extern TIM_HandleTypeDef htim4;
QueueHandle_t xLEDQueue;
SemaphoreHandle_t xLEDMutex;

void LED_Init()
{
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_3);
	HAL_TIM_PWM_Start(&htim4, TIM_CHANNEL_4);
}

void LED_OS_Resources_Init()
{
	xLEDQueue = xQueueCreate(4, sizeof(LEDMsgStruct));
	xLEDMutex = xSemaphoreCreateMutex();
}

void setColor(uint8_t r, uint8_t g, uint8_t b) {
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_1, r);
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_3, g);
	__HAL_TIM_SET_COMPARE(&htim4, TIM_CHANNEL_4, b);
}

bool auto_mode = false;
void LEDHandler()
{
	LEDMsgStruct LEDMsg;
	while(1)
	{
		if (xQueueReceive(xLEDQueue, &LEDMsg, portMAX_DELAY) == pdPASS) {
			if(LEDMsg.r < 0 || LEDMsg.g < 0 || LEDMsg.b < 0) {
				auto_mode = true;
			}
			else {
				auto_mode = false;
				if (xSemaphoreTake(xLEDMutex, portMAX_DELAY) == pdPASS) {
					setColor(LEDMsg.r, LEDMsg.g, LEDMsg.b);
					xSemaphoreGive(xLEDMutex);
				}

			}
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

void LEDTask()
{
	setColor(100, 100, 100);
	while(1)
	{
		if (xSemaphoreTake(xLEDMutex, portMAX_DELAY) == pdPASS) {
			if(auto_mode) {
				setColor(255, 0, 0);     // 紅
				HAL_Delay(100);
				setColor(255, 165, 0);   // 橙
				HAL_Delay(100);
				setColor(255, 255, 0);   // 黃
				HAL_Delay(100);
				setColor(0, 255, 0);     // 綠
				HAL_Delay(100);
				setColor(0, 127, 255);     // 藍
				HAL_Delay(100);
				setColor(0, 0, 255);    // 靛
				HAL_Delay(100);
				setColor(50, 0, 255);   // 紫
				HAL_Delay(100);
			}
			xSemaphoreGive(xLEDMutex);
		}
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}
