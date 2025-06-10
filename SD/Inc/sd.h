/*
 * sd.h
 *
 *  Created on: Jun 2, 2025
 *      Author: JasonHong
 */

#ifndef INC_SD_H_
#define INC_SD_H_

#include "fatfs.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "FreeRTOS.h"
#include "stm32f4xx_hal.h"
#include "queue.h"

#define FILEMGR_LIST_DEPDTH                        24
#define FILEMGR_FILE_NAME_SIZE                     40
#define FILEMGR_FULL_PATH_SIZE                     256
#define FILEMGR_MAX_LEVEL                          4
#define FILETYPE_DIR                               0
#define FILETYPE_FILE                              1

typedef struct _FILELIST_LineTypeDef {
  uint8_t type;
  uint8_t name[FILEMGR_FILE_NAME_SIZE];
}FILELIST_LineTypeDef;

typedef struct _FILELIST_FileTypeDef {
  FILELIST_LineTypeDef  file[FILEMGR_LIST_DEPDTH] ;
  uint16_t              ptr;
}FILELIST_FileTypeDef;

typedef struct
{
	char msg[100];
} SDMsgStruct;

void SD_Init(UART_HandleTypeDef *huart);
void SD_OS_Resources_Init();

void Mount_SD (void);
void Unmount_SD (void);

uint16_t GetFileNumber(void);
void SDParseHandler(void *pvParameters);

#endif /* INC_SD_H_ */
