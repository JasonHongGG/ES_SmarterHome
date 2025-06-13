/*
 * player.c
 *
 *  Created on: May 29, 2025
 *      Author: JasonHong
 */


#include "player.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "datas.h"
#include "msgHandler.h"


#define	WM8978_ADDRESS				0x1A
#define	WM8978_WIRTE_ADDRESS		(WM8978_ADDRESS << 1 | 0)
#define	BUFFER_SIZE					2048

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern I2S_HandleTypeDef hi2s3;
static I2C_HandleTypeDef* player_hi2s = &hi2c2;
extern UART_HandleTypeDef huart2;

HAL_StatusTypeDef WM8978_Register_Wirter(uint8_t reg_addr, uint16_t data)
{
	uint8_t pData[10] = {0};

	pData[0] = (reg_addr << 1) | ((data >> 8) & 0x01);
	pData[1] = data & 0xFF;
	return HAL_I2C_Master_Transmit(player_hi2s, WM8978_WIRTE_ADDRESS, pData, 2, 1000);
}

void my_WM8978_Init(void)
{
    HAL_StatusTypeDef status;
    SendMsg(&huart2, "my_WM8978_Init: Start\r\n");

    // if(__HAL_I2C_GET_FLAG(player_hi2s, I2C_FLAG_BUSY))
    // {
    //     __HAL_I2C_DISABLE(player_hi2s);
    //     player_hi2s->Instance->CR1 |= I2C_CR1_SWRST;  // 進行 SWRST
    //     player_hi2s->Instance->CR1 &= ~I2C_CR1_SWRST; // 清除 SWRST
    //     __HAL_I2C_ENABLE(player_hi2s);
    // }
    SendMsg(&huart2, "I2C2 State: %d\r\n", player_hi2s->State);

    SendMsg(&huart2, "my_WM8978_Init: Checking I2C device readiness...\r\n");

	status = HAL_I2C_IsDeviceReady(player_hi2s, WM8978_WIRTE_ADDRESS, 3, 100); // 嘗試3次，超時100ms
	if (status != HAL_OK) {
		SendMsg(&huart2, "my_WM8978_Init: WM8978 not ready on I2C bus! Status: %d  (%2d)\r\n", status, WM8978_WIRTE_ADDRESS);
		// 在這裡可以考慮直接返回，因為後續操作都會失敗
		//return;
	} else {
		SendMsg(&huart2, "my_WM8978_Init: WM8978 I2C device ready.\r\n");
	}


    SendMsg(&huart2, "my_WM8978_Init: Writing Reg 0 (Reset)\r\n");
    status = WM8978_Register_Wirter(0, 0);		// 软复位
    if (status != HAL_OK) {
        SendMsg(&huart2, "my_WM8978_Init: Reg 0 FAILED! Status: %d\r\n", status);
    } else {
        SendMsg(&huart2, "my_WM8978_Init: Reg 0 OK\r\n");
    }
    HAL_Delay(50); // Short delay after reset

    SendMsg(&huart2, "my_WM8978_Init: Writing Reg 1\r\n");
    status = WM8978_Register_Wirter(1, 0x0F);	// 模拟放大器使能， 使能输出输入缓存区
    if (status != HAL_OK) {
        SendMsg(&huart2, "my_WM8978_Init: Reg 1 FAILED! Status: %d\r\n", status);
    } else {
        SendMsg(&huart2, "my_WM8978_Init: Reg 1 OK\r\n");
    }

    SendMsg(&huart2, "my_WM8978_Init: Writing Reg 3\r\n");
    status = WM8978_Register_Wirter(3, 0x7F);	// 使能左右声道和L\ROUT2
    if (status != HAL_OK) {
        SendMsg(&huart2, "my_WM8978_Init: Reg 3 FAILED! Status: %d\r\n", status);
    } else {
        SendMsg(&huart2, "my_WM8978_Init: Reg 3 OK\r\n");
    }

    // ... Continue this pattern for all WM8978_Register_Wirter calls ...

    SendMsg(&huart2, "my_WM8978_Init: Writing Reg 55\r\n");
    status = WM8978_Register_Wirter(55,30|(1<<8));	// 设置ROUT2右声道音量， 更新左右声道音量
    if (status != HAL_OK) {
        SendMsg(&huart2, "my_WM8978_Init: Reg 55 FAILED! Status: %d\r\n", status);
    } else {
        SendMsg(&huart2, "my_WM8978_Init: Reg 55 OK\r\n");
    }
    SendMsg(&huart2, "my_WM8978_Init: End\r\n");
}

void WM8978_Palyer(void)
{
	SendMsg(&huart2, "WM8978_Player\r\n");
	uint32_t DataLength = 0;
	uint8_t* DataAddress = NULL;
	uint16_t* TempAddress = NULL;

	DataLength = sizeof(data) - 0x2c;
	DataAddress = (unsigned char *)(data + 0x2c);
	TempAddress = (uint16_t*)DataAddress;

	HAL_StatusTypeDef i2s_status;
	while(1)
	{
		if (DataLength == 0) break; // No more data

        uint16_t current_chunk_size_samples = BUFFER_SIZE / 2; // BUFFER_SIZE is in bytes, HAL_I2S_Transmit takes number of samples (uint16_t)
        if (DataLength < BUFFER_SIZE) {
            current_chunk_size_samples = DataLength / 2; // Remaining samples
            if (DataLength % 2 != 0) {
                SendMsg(&huart2, "Warning: Odd DataLength remaining, potential data truncation.\r\n");
                // Handle odd byte count if necessary, though I2S usually deals in words
            }
        }

        if (current_chunk_size_samples == 0 && DataLength > 0) {
            // This case might happen if DataLength is 1, for example.
            // SendMsg(&huart2, "Warning: current_chunk_size_samples is 0 but DataLength is %lu. Breaking.\r\n", DataLength);
            break;
        }
        if (current_chunk_size_samples == 0 && DataLength == 0) {
            break; // Normal end of loop if all data processed
        }

        // SendMsg(&huart2, "Transmitting %u samples. Remaining DataLength: %lu\r\n", current_chunk_size_samples, DataLength);
        i2s_status = HAL_I2S_Transmit(&hi2s3, TempAddress, current_chunk_size_samples, 1000);
        if (i2s_status != HAL_OK)
        {
            SendMsg(&huart2, "HAL_I2S_Transmit failed! Error: %d\r\n", i2s_status);
            break; // 传输失败，退出循环
        }

        if (DataLength < BUFFER_SIZE) {
            DataLength = 0; // All remaining data sent
            break;
        } else {
            DataLength -= BUFFER_SIZE;
            TempAddress += (BUFFER_SIZE / 2); // Increment by number of uint16_t samples
        }
	}
}
