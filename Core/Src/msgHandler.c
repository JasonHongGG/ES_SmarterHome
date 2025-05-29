#include "msgHandler.h"
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "stm32f4xx_hal.h"
#include "semphr.h"
#include "msgHandler.h"

UART_HandleTypeDef* msgHandler_esp32_huart;
UART_HandleTypeDef* msgHandler_shell_huart;
SemaphoreHandle_t msgHandlerTx;
SemaphoreHandle_t msgHandlerRx;
SemaphoreHandle_t ReceiveMsgTimeoutMutex;
int ReceiveMsgTimeout = 10;

void MsgHandler_Init(UART_HandleTypeDef* shell_huart)
{
	msgHandler_shell_huart = shell_huart;
}

void MsgHandler_OS_Resources_Init()
{
	msgHandlerTx = xSemaphoreCreateMutex();
	msgHandlerRx = xSemaphoreCreateMutex();
	ReceiveMsgTimeoutMutex = xSemaphoreCreateMutex();
}

void SendChar(UART_HandleTypeDef* huart, const char c)
{
	SendMsg(huart, "%c", c);
}

void SendMsg(UART_HandleTypeDef* huart, const char* fmt, ...)
{
	if(msgHandlerTx){
		if (xSemaphoreTake(msgHandlerTx, portMAX_DELAY) == pdPASS) {
			static char send_data[256];
			va_list args;
			va_start(args, fmt);
			vsnprintf(send_data, sizeof(send_data), fmt, args);
			va_end(args);
			HAL_UART_Transmit(huart, (uint8_t *)send_data, strlen(send_data), HAL_MAX_DELAY);

			xSemaphoreGive(msgHandlerTx);
		}
	}
	else{
		static char send_data[256];
		va_list args;
		va_start(args, fmt);
		vsnprintf(send_data, sizeof(send_data), fmt, args);
		va_end(args);
		HAL_UART_Transmit(huart, (uint8_t *)send_data, strlen(send_data), HAL_MAX_DELAY);

	}
}

int ReceiveChar(UART_HandleTypeDef* huart, char* receive_data)
{
	if (xSemaphoreTake(msgHandlerRx, portMAX_DELAY) == pdPASS) {
		if (HAL_UART_Receive(huart, (uint8_t *)receive_data, 1, 500) == HAL_OK) {
			xSemaphoreGive(msgHandlerRx);
			return HAL_OK;
		}
		xSemaphoreGive(msgHandlerRx);
	}

	return HAL_ERROR;
}

int ReceiveMsg(UART_HandleTypeDef* huart, char* receive_data, size_t buffer_size)
{
    memset(receive_data, '\0', buffer_size-1);
    if (xSemaphoreTake(msgHandlerRx, portMAX_DELAY) == pdPASS) {


		if (xSemaphoreTake(ReceiveMsgTimeoutMutex, portMAX_DELAY) == pdPASS) {
			HAL_StatusTypeDef result = HAL_UART_Receive(huart, (uint8_t *)receive_data, buffer_size-1, ReceiveMsgTimeout);
			ReceiveMsgTimeout = 500;
			xSemaphoreGive(ReceiveMsgTimeoutMutex);
			if (result == HAL_OK) {
				xSemaphoreGive(msgHandlerRx);
				return HAL_OK;
			}
		}

		xSemaphoreGive(msgHandlerRx);
    }

	return HAL_ERROR;
}



//int ReceiveChar(UART_HandleTypeDef* huart, char* receive_data)
//{
//	SemaphoreHandle_t* rxMutex = &msgHandlerRx;
//	if (xSemaphoreTake(*rxMutex, portMAX_DELAY) == pdPASS) {
//		SendMsg(msgHandler_shell_huart, "ReceiveChar : Get rxMutex\r\n");
//		if (HAL_UART_Receive(huart, (uint8_t *)receive_data, 1, 500) == HAL_OK) {
//			SendMsg(msgHandler_shell_huart, "ReceiveChar : Release rxMutex\r\n");
//			xSemaphoreGive(*rxMutex);
//			return HAL_OK;
//		}
//		SendMsg(msgHandler_shell_huart, "ReceiveChar : Release rxMutex\r\n");
//		xSemaphoreGive(*rxMutex);
//	}
//	else {
//		SendMsg(msgHandler_shell_huart, "ReceiveChar : Release Release rxMutex\r\n");
//	}
//
//	return HAL_ERROR;
//}
//
//int ReceiveMsg(UART_HandleTypeDef* huart, char* receive_data, size_t buffer_size)
//{
//	SendMsg(msgHandler_shell_huart, "ReceiveMsg\r\n");
//	SemaphoreHandle_t* rxMutex = &msgHandlerRx;
//    memset(receive_data, '\0', buffer_size-1);
//    if (xSemaphoreTake(*rxMutex, portMAX_DELAY) == pdPASS) {
//    	SendMsg(msgHandler_shell_huart, "ReceiveMsg : Get rxMutex\r\n");
//		if (HAL_UART_Receive(huart, (uint8_t *)receive_data, buffer_size-1, 500) == HAL_OK) {
//			SendMsg(msgHandler_shell_huart, "ReceiveMsg : Release rxMutex\r\n");
//			xSemaphoreGive(*rxMutex);
//			return HAL_OK;
//		}
//		SendMsg(msgHandler_shell_huart, "ReceiveMsg : Release rxMutex\r\n");
//		xSemaphoreGive(*rxMutex);
//    }
//
//	return HAL_ERROR;
//}


