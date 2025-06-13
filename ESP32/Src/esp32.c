/*
 * esp32.c
 *
 *  Created on: May 17, 2025
 *      Author: JasonHong
 */
#include "esp32.h"
#include "msgHandler.h"
#include "shell.h"


UART_HandleTypeDef* eps32_TxRx_huart;
UART_HandleTypeDef* esp32_log_huart;
QueueHandle_t xESP32Queue;
QueueHandle_t xESP32ReceiverQueue;
SemaphoreHandle_t xESP32Mutex;
extern SemaphoreHandle_t ReceiveMsgTimeoutMutex;
extern QueueHandle_t xShellQueue;
char ESP32_reveice_data[100];

void ESP32_Init(UART_HandleTypeDef* eps32_huart, UART_HandleTypeDef* log_huart)
{
	eps32_TxRx_huart = eps32_huart;
	esp32_log_huart = log_huart;
}

void ESP32_OS_Resources_Init()
{
	xESP32Queue = xQueueCreate(4, sizeof(ESP32MsgStruct));
	xESP32ReceiverQueue = xQueueCreate(4, sizeof(ESP32MsgStruct));
	xESP32Mutex = xSemaphoreCreateMutex();

	// 啟動 Idle-Line DMA 接收
    HAL_UARTEx_ReceiveToIdle_DMA(eps32_TxRx_huart,
                                 (uint8_t*)ESP32_reveice_data,
                                 sizeof(ESP32_reveice_data));
    // 啟用半滿中斷 (選擇性，用於即時 debug)
    // __HAL_DMA_ENABLE_IT(eps32_TxRx_huart->hdmarx, DMA_IT_HT);
	__HAL_DMA_DISABLE_IT(eps32_TxRx_huart->hdmarx, DMA_IT_HT);
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

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size)
{
  if (huart == eps32_TxRx_huart)
  {
    ESP32MsgStruct msg;
	memset(msg.msg, 0, sizeof(msg.msg));
    memcpy(msg.msg, ESP32_reveice_data, Size);
    msg.msg[Size] = '\0';

    // 從 ISR 推送到 Queue
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	xQueueSendFromISR(xESP32ReceiverQueue, &msg, &xHigherPriorityTaskWoken);
	portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

	memset(ESP32_reveice_data, 0, sizeof(ESP32_reveice_data));
    HAL_UARTEx_ReceiveToIdle_DMA(eps32_TxRx_huart,
                                 (uint8_t*)ESP32_reveice_data,
                                 sizeof(ESP32_reveice_data));
	__HAL_DMA_DISABLE_IT(eps32_TxRx_huart->hdmarx, DMA_IT_HT);
	
  }
}

// void ESP32Receiver(void *pvParameters)
// {
//   while (1)
//   {
// 	memset(ESP32_reveice_data, '\0', sizeof(ESP32_reveice_data));
//     if (ReceiveMsg(eps32_TxRx_huart, ESP32_reveice_data, sizeof(ESP32_reveice_data)) == HAL_OK)
//     {
//       SendMsg(esp32_log_huart, "\n\rReceive : ESP32: %s\n\r", ESP32_reveice_data);  // TAG:Debug

// //      for(int i = 0 ; i < 100; i++) {
// //    	  if(ESP32_reveice_data[i] == " ")
// //    		  SendMsg(esp32_log_huart, "\\s");  // TAG:Debug
// //    	  else if (ESP32_reveice_data[i] == "\0")
// //    		  SendMsg(esp32_log_huart, "\\0");
// //    	  else
// //    		  SendMsg(esp32_log_huart, "%c(%d)", ESP32_reveice_data[i], ESP32_reveice_data[i]);
// //      }
//     }
//     vTaskDelay(pdMS_TO_TICKS(2));
//   }
// }

void ESP32Receiver(void *pvParameters)
{
  ESP32MsgStruct rxMsg;
  while (1)
  {
    if (xQueueReceive(xESP32ReceiverQueue, &rxMsg, portMAX_DELAY) == pdPASS)
    {
    	SendMsg(esp32_log_huart, "\r\nReceive : ESP32: %s\r\n", rxMsg.msg);
		
    	ShellMsgStruct shellMsg;
    	strncpy(shellMsg.msg, rxMsg.msg, sizeof(rxMsg.msg)-1);
    	shellMsg.msg[sizeof(shellMsg.msg)-1] = '\0';
    	if (xQueueSend(xShellQueue, &shellMsg, 0) != pdPASS) {
			SendMsg(esp32_log_huart, "\r\ESP32Receiver: Queue full or error.\r\n");
		}
    }
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
