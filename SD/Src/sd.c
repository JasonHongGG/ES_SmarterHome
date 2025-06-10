/*
 * sd.c
 *
 *  Created on: Jun 2, 2025
 *      Author: JasonHong
 */

#include "sd.h"
#include "msgHandler.h"

extern FILELIST_FileTypeDef FileList;
UART_HandleTypeDef* sd_huart;
FILELIST_FileTypeDef FileList;
QueueHandle_t xSDQueue;

FATFS fs;
uint16_t NumObs = 0;

void SD_Init(UART_HandleTypeDef *huart)
{
	sd_huart = huart;
	Mount_SD();
	GetFileNumber();
}

void SD_OS_Resources_Init()
{
	xSDQueue = xQueueCreate(4, sizeof(SDMsgStruct));
}


FRESULT StorageParse(const char *pRequestedExtension)
{
  FRESULT res = FR_OK;
  FILINFO fno;
  DIR dir;
  char *fn;
  char *ext_dot;

  char lower_req_ext[16]; // Buffer for lowercase requested extension
  char file_ext_lower[16]; // Buffer for lowercase file extension

  if (pRequestedExtension == NULL || strlen(pRequestedExtension) == 0 || strlen(pRequestedExtension) >= sizeof(lower_req_ext)) {
      SendMsg(sd_huart, "StorageParse: Invalid or too long extension provided.\r\n");
      return FR_INVALID_PARAMETER;
  }

  // Convert requested extension to lowercase for consistent comparison
  strncpy(lower_req_ext, pRequestedExtension, sizeof(lower_req_ext) - 1);
  lower_req_ext[sizeof(lower_req_ext) - 1] = '\0';
  for (int i = 0; lower_req_ext[i]; i++) {
    lower_req_ext[i] = tolower((unsigned char)lower_req_ext[i]);
  }

  SendMsg(sd_huart, "\r\nParsing SD card for *.%s files...\r\n", lower_req_ext);

  res = f_opendir(&dir, "");
  if (res != FR_OK) {
      SendMsg(sd_huart, "Failed to open root directory! Error: %d\r\n", res);
      return res;
  }
  FileList.ptr = 0; // Reset file list for each parse

  if(res == FR_OK)
  {
    while(1)
    {
      res = f_readdir(&dir, &fno);
      if(res != FR_OK || fno.fname[0] == 0)
      {
        break;
      }
      if(fno.fname[0] == '.') // Skip hidden files/directories and current/parent dir entries
      {
        continue;
      }

      fn = fno.fname;

      if(FileList.ptr < FILEMGR_LIST_DEPDTH)
      {
        if((fno.fattrib & AM_DIR) == 0) // If it's a file
        {
          ext_dot = strrchr(fn, '.'); // Find the last dot
          // Ensure a dot is found, it's not the first character (e.g. ".hiddenfile"),
          // and there's something after the dot.
          if (ext_dot != NULL && ext_dot != fn && *(ext_dot + 1) != '\0') {
            strncpy(file_ext_lower, ext_dot + 1, sizeof(file_ext_lower) - 1);
            file_ext_lower[sizeof(file_ext_lower) - 1] = '\0';
            for (int i = 0; file_ext_lower[i]; i++) {
                file_ext_lower[i] = tolower((unsigned char)file_ext_lower[i]);
            }

            if (strcmp(file_ext_lower, lower_req_ext) == 0)
            {
              strncpy((char *)FileList.file[FileList.ptr].name, (char *)fn, FILEMGR_FILE_NAME_SIZE);
              FileList.file[FileList.ptr].type = FILETYPE_FILE; // Assuming FILETYPE_FILE is generic
              SendMsg(sd_huart, "Found *.%s file: %s\r\n", lower_req_ext, fn);
              FileList.ptr++;
            }
          }
        }
      } else {
        SendMsg(sd_huart, "File list full. Cannot add more files.\r\n");
        break; // Stop if file list is full
      }
    }
  }
  NumObs = FileList.ptr;
  f_closedir(&dir);
  SendMsg(sd_huart, "Total *.%s files found: %d\r\n", lower_req_ext, NumObs);
  return res;
}

uint16_t GetFileNumber(void)
{
	if (NumObs) return NumObs;
	else return -1;
}

void SDParseHandler(void *pvParameters)
{
  SDMsgStruct sdMsg;
  while (1)
  {
	if (xQueueReceive(xSDQueue, &sdMsg, portMAX_DELAY) == pdPASS) {
		StorageParse(sdMsg.msg);
//		SendMsg(sd_huart, "SDParseHandler\r\n");
	}
	 vTaskDelay(pdMS_TO_TICKS(500));
  }
}

void Mount_SD (void)
{
	FRESULT fresult = f_mount(&fs, "", 1);

	if (fresult != FR_OK) {
		SendMsg(sd_huart, "SD card mount failed with error code: %d\r\n", fresult);
	} else {
		SendMsg(sd_huart, "SD card mounted Successfully.!\r\n");
	}
}

void Unmount_SD (void)
{
	FRESULT fresult = f_mount(NULL, "", 1);

	if (fresult != FR_OK) {
		SendMsg(sd_huart, "SD card unmount failed with error code: %d\r\n", fresult);
	} else {
		SendMsg(sd_huart, "SD card unmounted Successfully.!\r\n");
	}
}

