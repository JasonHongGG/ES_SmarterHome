/*
 * readLine.cpp
 *
 *  Created on: May 14, 2025
 *      Author: JasonHong
 */
#include "readLine.h"
#include "msgHandler.h"
static char prompt[PROMPT_LEN] = "> ";
extern UART_HandleTypeDef* shell_huart;

void ReadLine_Init(UART_HandleTypeDef* huart)
{
	shell_huart = huart;
}

static bool GetChar(char *out_char)
{
	if(ReceiveChar(shell_huart, out_char) == HAL_OK)
		return true;
	return false;
}

static void PutChar(char cChar)
{
	SendChar(shell_huart, cChar);
}

static void PutString(const char *const pcChar)
{
	SendMsg(shell_huart, pcChar);
}

uint32_t ReadLine(char* readBuffer, int bufferSize)
{

	char *pCur = readBuffer; /* the point to ready receive. */
	char cChar = 0;
	uint32_t receiveCharCnt = 0;
	uint32_t PromptLen = 0;
	uint32_t outputColumnCnt = 0;
	memset(readBuffer, 0, bufferSize);
	/* print Prompt */
	if (*prompt)
	{
		PutString("\n\r");
		PromptLen = strlen(prompt);
		outputColumnCnt = PromptLen;
		PutString(prompt);
	}
	for (;;)
	{
		bool isGotChar = GetChar(&cChar);

		if(isGotChar) {
			switch (cChar)
			{
			case 0x03: /* ^C - break */
				return 0;
			case '\r': /*\r*/
			case '\n': /*\n*/
				if ((pCur >= &readBuffer[0]) &&
					(pCur <= &readBuffer[READBUF_LEN]))
				{
					*(pCur + 1) = '\0';
	//				PutString("\n\r");
	//				PutString(readBuffer);
					return (pCur - readBuffer);
				}
				else /* Buffer full (Overflow) */
				{
					memset(readBuffer, 0, bufferSize);
					return 0;
				}

			case 0x15: /* ^U - erase line */
				while (outputColumnCnt > PromptLen)
				{
					outputColumnCnt--;
				}
				memset(readBuffer, 0, bufferSize);
				pCur = readBuffer;
				receiveCharCnt = 0x00;
				PutString("\n\r> ");
				break;

			case 0x7F: /* DEL - backspace	*/
				// 鼠標位置大於 promote
				if ((outputColumnCnt > PromptLen) && (pCur > &readBuffer[0]) && (receiveCharCnt > 0x00))
				{
					outputColumnCnt--;
					pCur--;
					receiveCharCnt--;
					*pCur = '\0'; /* earse the receice char */
					PutString("\n\r> ");
					PutString(readBuffer);
				}
				break;

			default:
				if (receiveCharCnt < bufferSize &&
					(pCur >= &readBuffer[0]) &&
					(pCur <= &readBuffer[READBUF_LEN]) &&
					(cChar > 0x19 && cChar < 0x7F))
				{
					outputColumnCnt++;
					PutChar(cChar);
					*pCur = cChar;
					pCur++;
					receiveCharCnt++;
				}
				else /* Buffer full (Overflow) */
				{
					PutChar('\a');
				}
			}
			vTaskDelay(pdMS_TO_TICKS(0));
		}
		else {
			vTaskDelay(pdMS_TO_TICKS(50));
		}
	}
	return 0;
}

bool ArgAnalyze(char* readBuffer, uint8_t *argc, char *argv[])
{
	uint32_t i = 0;

	while (*argc < MAX_ARGS)
	{
		/* skip any separator */
		while (readBuffer[i] == ' ')
		{
			i++;
		}
		/* end of line, no more args	*/
		if (readBuffer[i] == '\0')
		{
			argv[*argc] = NULL;
			return true;
		}
		/* begin of argument string */
		argv[(*argc)++] = &readBuffer[i];
		/* find end of argument string */
		while (readBuffer[i] && readBuffer[i] != ' ')
		{
			i++;
		}
		/* end of line, no more args */
		if (readBuffer[i] == '\0')
		{
			argv[*argc] = NULL;
			return true;
		}
		readBuffer[i++] = '\0';
	}
	return false;
}
