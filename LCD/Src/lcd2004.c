#include "lcd2004.h"
#include "msgHandler.h"


QueueHandle_t xLCDQueue;
SemaphoreHandle_t xLCDMutex;
I2C_LCD_HandleTypeDef lcd;
UART_HandleTypeDef* lcd_huart;
extern RTC_HandleTypeDef hrtc;

RTC_TimeTypeDef sTime;
RTC_DateTypeDef sDate;

void LCD2004_Init(I2C_HandleTypeDef *hi2c, uint8_t address, UART_HandleTypeDef* haurt)
{
  lcd.hi2c = hi2c;
  lcd.address = address; //0x4E（0x27 << 1）
  lcd_init(&lcd);
  lcd_huart = haurt;
}

void LCD2004_OS_Resources_Init()
{
  xLCDQueue = xQueueCreate(4, sizeof(LCDMsgStruct));
  xLCDMutex = xSemaphoreCreateMutex();
}

void LCD_ClearLine(uint8_t row)
{
    lcd_gotoxy(&lcd, 0, row);
    for (int i = 0; i < 20; i++) {
        lcd_putchar(&lcd, ' ');
    }
}

void SetLCDCommandStatus(char* str)
{
	LCD_ClearLine(0);
	LCD_ClearLine(1);
	LCD_ClearLine(3);

	LCDMsgStruct commandMsg;
	commandMsg.row = 0;
	commandMsg.col = 0;
	char buf[50];
	sprintf(buf, "Command : %s", str);
	strncpy(commandMsg.msg, buf, sizeof(commandMsg.msg)-1);
	commandMsg.msg[sizeof(commandMsg.msg)-1] = '\0';

	if (xQueueSend(xLCDQueue, &commandMsg, 0) != pdPASS) {
		SendMsg(lcd_huart, "\r\nLCDShowMsg: Queue full or error.\r\n");
	}

	LCDMsgStruct statusMSg;
	statusMSg.row = 1;
	statusMSg.col = 0;
	strncpy(statusMSg.msg, "Status : Success", sizeof(statusMSg.msg)-1);
	statusMSg.msg[sizeof(statusMSg.msg)-1] = '\0';

	if (xQueueSend(xLCDQueue, &statusMSg, 0) != pdPASS) {
		SendMsg(lcd_huart, "\r\nLCDShowMsg: Queue full or error.\r\n");
	}



	LCDMsgStruct timeMSg;
	timeMSg.row = 3;
	timeMSg.col = 0;
	HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN);
	HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
	snprintf(timeMSg.msg, sizeof(timeMSg.msg), "%02d/%02d %02d:%02d:%02d",
			sDate.Month, sDate.Date, sTime.Hours, sTime.Minutes, sTime.Seconds);
	timeMSg.msg[sizeof(timeMSg.msg)-1] = '\0';

	if (xQueueSend(xLCDQueue, &timeMSg, 0) != pdPASS) {
		SendMsg(lcd_huart, "\r\nLCDShowMsg: Queue full or error.\r\n");
	}
}

void LCDHandler(void *pvParameters)
{
  LCDMsgStruct lcdMsg;
  while (1)
  {
      if (xQueueReceive(xLCDQueue, &lcdMsg, portMAX_DELAY) == pdPASS) {
//        if (xSemaphoreTake(xLCDMutex, portMAX_DELAY) == pdPASS) {
//          //SendMsg(&huart2, "\n\r LCDReceiveHandler : %d %d %s\n\r", lcdMsg.row, lcdMsg.col, lcdMsg.msg);
//
//          char* line = lines[lcdMsg.row];
//          for(int i = 0 ; i < strlen(lcdMsg.msg) ; i++) {
//            line[lcdMsg.col + i] = lcdMsg.msg[i];
//          }
//
//          xSemaphoreGive(xLCDMutex);
//	    }
    	  //SendMsg(&huart2, "\n\r LCDReceiveHandler : %d %d %s\n\r", lcdMsg.row, lcdMsg.col, lcdMsg.msg);
          lcd_gotoxy(&lcd, lcdMsg.col, lcdMsg.row);
          lcd_puts(&lcd, lcdMsg.msg);
      }
      vTaskDelay(pdMS_TO_TICKS(500));
  }
}


char lines[4][20] = {
      "Line 1: Hello STM32",
      "Line 2: I2C LCD2004",
      "Line 3: Works fine!",
      "Line 4: Yay :)"};

void LCDTemplateHandler(void *pvParameters)
{
  int current_line = 0;

  while (1)
  {
    if (xSemaphoreTake(xLCDMutex, portMAX_DELAY) == pdPASS) {
      // show
      for (int i = 0; i < 4; i++)
      {
        lcd_gotoxy(&lcd, 0, i);
        lcd_puts(&lcd, lines[i]);

        lcd_gotoxy(&lcd, 19, i);
        if (i == current_line)
          lcd_putchar(&lcd, '*');
        else
          lcd_putchar(&lcd, ' ');
      }
      xSemaphoreGive(xLCDMutex);
    }

    current_line = (current_line + 1) % 4;
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
