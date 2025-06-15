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

#define CMD_HISTORY_COUNT  		2
#define CMD_HISTORY_MAX_LEN    64
static char historyCommand[CMD_HISTORY_COUNT][CMD_HISTORY_MAX_LEN];
static int  head = 0;    // 下一筆寫入索引
static int  cnt = 0;     // 已存筆數 (<= SHELL_HISTORY_COUNT)
static int  pos = -1;     // 瀏覽指標 (-1 表示未在瀏覽)


void ReadLine_Init(UART_HandleTypeDef* huart)
{
	shell_huart = huart;
	for(int i=0;i<CMD_HISTORY_COUNT;i++) historyCommand[i][0]=0;
}

void CommandHistory_Add(const char *cmd)
{
    if (*cmd == '\0') return;
    // 避免重複同一筆
    if (cnt>0) {
        int last = (head+CMD_HISTORY_COUNT-1)%CMD_HISTORY_COUNT;
        if (strcmp(historyCommand[last], cmd)==0) return;
    }
    strncpy(historyCommand[head], cmd, CMD_HISTORY_MAX_LEN-1);
    historyCommand[head][CMD_HISTORY_MAX_LEN-1] = '\0';
    head = (head+1) % CMD_HISTORY_COUNT;
    if (cnt < CMD_HISTORY_COUNT) cnt++;
    pos = -1;
}

bool CommandHistory_Prev(char *out)
{
    if (cnt==0) return false;
    if (pos < 0) {
        pos = (head+CMD_HISTORY_COUNT-1)%CMD_HISTORY_COUNT;
    } else {
        pos = (pos+CMD_HISTORY_COUNT-1)%CMD_HISTORY_COUNT;
    }
    strcpy(out, historyCommand[pos]);
    return true;
}

bool CommandHistory_Next(char *out)
{
    if (cnt==0 || pos<0) return false;
    pos = (pos+1) % CMD_HISTORY_COUNT;
    if (pos == head) {
        out[0] = '\0';
        pos = -1;
        return true;
    }
    strcpy(out, historyCommand[pos]);
    return true;
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
					CommandHistory_Add(readBuffer);
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
			case 0x08:
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

			case '[':
				char preCmd[CMD_HISTORY_MAX_LEN];
				if(!CommandHistory_Prev(preCmd)) break;
				PutString("\n\r> ");
				PutString(preCmd);
				strncpy(readBuffer, preCmd, sizeof(preCmd));
				int preCmdLen = strlen(readBuffer);
				pCur = &readBuffer[preCmdLen];
				outputColumnCnt = PromptLen + preCmdLen;
				receiveCharCnt = preCmdLen;
				break;

			case ']':
				char nextCmd[CMD_HISTORY_MAX_LEN];
				if(!CommandHistory_Next(nextCmd)) break;
				PutString("\n\r> ");
				PutString(nextCmd);
				strncpy(readBuffer, nextCmd, sizeof(nextCmd));
				int nextCmdLen = strlen(readBuffer);
				pCur = &readBuffer[nextCmdLen];
				outputColumnCnt = PromptLen + nextCmdLen;
				receiveCharCnt = nextCmdLen;
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
