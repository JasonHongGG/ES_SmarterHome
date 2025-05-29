#ifndef INC_MYPRINTF_H_
#define INC_MYPRINTF_H_
#include "stm32f4xx_hal.h"
#include <stdint.h>

extern int ReceiveMsgTimeout;

void MsgHandler_Init();

void MsgHandler_OS_Resources_Init();

void SendChar(UART_HandleTypeDef* huart, const char c);

void SendMsg(UART_HandleTypeDef* huart, const char* fmt, ...);

int ReceiveChar(UART_HandleTypeDef* huart, char* receive_data);

int ReceiveMsg(UART_HandleTypeDef* huart, char* reveice_data, size_t buffer_size);

#endif /* INC_MYPRINTF_H_ */
