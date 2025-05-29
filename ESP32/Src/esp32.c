/*
 * esp32.c
 *
 *  Created on: May 17, 2025
 *      Author: JasonHong
 */
#include "esp32.h"
#include "msgHandler.h"


UART_HandleTypeDef* eps32_TxRx_huart;
UART_HandleTypeDef* esp32_log_huart;
QueueHandle_t xESP32Queue;
SemaphoreHandle_t xESP32Mutex;
extern SemaphoreHandle_t ReceiveMsgTimeoutMutex;
char ESP32_reveice_data[100];

void ESP32_Init(UART_HandleTypeDef* eps32_huart, UART_HandleTypeDef* log_huart)
{
	eps32_TxRx_huart = eps32_huart;
	esp32_log_huart = log_huart;
}

void ESP32_OS_Resources_Init()
{
	xESP32Queue = xQueueCreate(4, sizeof(ESP32MsgStruct));
	xESP32Mutex = xSemaphoreCreateMutex();
}

void ESP32Sender(void *pvParameters)
{
  ESP32MsgStruct esp32Msg;
  while (1)
  {
	if (xQueueReceive(xESP32Queue, &esp32Msg, portMAX_DELAY) == pdPASS) {
	  	if (xSemaphoreTake(ReceiveMsgTimeoutMutex, portMAX_DELAY) == pdPASS) {
			ReceiveMsgTimeout = HAL_MAX_DELAY;

			xSemaphoreGive(ReceiveMsgTimeoutMutex);
		}
	  	SendMsg(eps32_TxRx_huart, esp32Msg.msg);
	}

	vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void ESP32Receiver(void *pvParameters)
{
  while (1)
  {
	memset(ESP32_reveice_data, '\0', sizeof(ESP32_reveice_data));
    if (ReceiveMsg(eps32_TxRx_huart, ESP32_reveice_data, sizeof(ESP32_reveice_data)) == HAL_OK)
    {
      SendMsg(esp32_log_huart, "\n\rReceive : ESP32: %s\n\r", ESP32_reveice_data);  // TAG:Debug

//      for(int i = 0 ; i < 100; i++) {
//    	  if(ESP32_reveice_data[i] == " ")
//    		  SendMsg(esp32_log_huart, "\\s");  // TAG:Debug
//    	  else if (ESP32_reveice_data[i] == "\0")
//    		  SendMsg(esp32_log_huart, "\\0");
//    	  else
//    		  SendMsg(esp32_log_huart, "%c(%d)", ESP32_reveice_data[i], ESP32_reveice_data[i]);
//      }
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
