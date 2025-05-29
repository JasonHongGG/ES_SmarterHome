#include "lcd2004.h"
#include "msgHandler.h"


QueueHandle_t xLCDQueue;
SemaphoreHandle_t xLCDMutex;
I2C_LCD_HandleTypeDef lcd;
extern UART_HandleTypeDef huart2;

void LCD2004_Init(I2C_HandleTypeDef *hi2c, uint8_t address)
{
  lcd.hi2c = hi2c;
  lcd.address = address; //0x4E（0x27 << 1）
  lcd_init(&lcd);
}

void LCD2004_OS_Resources_Init()
{
  xLCDQueue = xQueueCreate(4, sizeof(LCDMsgStruct));
  xLCDMutex = xSemaphoreCreateMutex();
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
