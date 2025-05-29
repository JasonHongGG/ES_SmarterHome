/**
 ******************************************************************************
  * @file    user_diskio_spi.c
  * @brief   This file contains the implementation of the user_diskio_spi FatFs
  *          driver.
  ******************************************************************************
  * Portions copyright (C) 2014, ChaN, all rights reserved.
  * Portions copyright (C) 2017, kiwih, all rights reserved.
  *
  * This software is a free software and there is NO WARRANTY.
  * No restriction on use. You can use, modify and redistribute it for
  * personal, non-profit or commercial products UNDER YOUR RESPONSIBILITY.
  * Redistributions of source code must retain the above copyright notice.
  *
  ******************************************************************************
  */

//This code was ported by kiwih from a copywrited (C) library written by ChaN
//available at http://elm-chan.org/fsw/ff/ffsample.zip
//(text at http://elm-chan.org/fsw/ff/00index_e.html)

//This file provides the FatFs driver functions and SPI code required to manage
//an SPI-connected MMC or compatible SD card with FAT

//It is designed to be wrapped by a cubemx generated user_diskio.c file.

#include "stm32f4xx_hal.h" /* Provide the low-level HAL functions */
#include "user_diskio_spi.h"

// hong
#include "msgHandler.h"
extern UART_HandleTypeDef huart2;
//Make sure you set #define SD_SPI_HANDLE as some hspix in main.h
//Make sure you set #define SD_CS_GPIO_Port as some GPIO port in main.h
//Make sure you set #define SD_CS_Pin as some GPIO pin in main.h
#include "main.h"
#define SD_SPI_HANDLE hspi2
extern SPI_HandleTypeDef SD_SPI_HANDLE;

/* Function prototypes */

//(Note that the _256 is used as a mask to clear the prescalar bits as it provides binary 111 in the correct position)
#define FCLK_SLOW() { MODIFY_REG(SD_SPI_HANDLE.Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_128); }	/* Set SCLK = slow, approx 280 KBits/s*/
#define FCLK_FAST() { MODIFY_REG(SD_SPI_HANDLE.Instance->CR1, SPI_BAUDRATEPRESCALER_256, SPI_BAUDRATEPRESCALER_8); }	/* Set SCLK = fast, approx 4.5 MBits/s */

#define CS_HIGH()	{HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET);}
#define CS_LOW()	{HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET);}

/*--------------------------------------------------------------------------

   Module Private Functions

---------------------------------------------------------------------------*/

/* MMC/SD command */
#define CMD0	(0)			/* GO_IDLE_STATE */
#define CMD1	(1)			/* SEND_OP_COND (MMC) */
#define	ACMD41	(0x80+41)	/* SEND_OP_COND (SDC) */
#define CMD8	(8)			/* SEND_IF_COND */
#define CMD9	(9)			/* SEND_CSD */
#define CMD10	(10)		/* SEND_CID */
#define CMD12	(12)		/* STOP_TRANSMISSION */
#define ACMD13	(0x80+13)	/* SD_STATUS (SDC) */
#define CMD16	(16)		/* SET_BLOCKLEN */
#define CMD17	(17)		/* READ_SINGLE_BLOCK */
#define CMD18	(18)		/* READ_MULTIPLE_BLOCK */
#define CMD23	(23)		/* SET_BLOCK_COUNT (MMC) */
#define	ACMD23	(0x80+23)	/* SET_WR_BLK_ERASE_COUNT (SDC) */
#define CMD24	(24)		/* WRITE_BLOCK */
#define CMD25	(25)		/* WRITE_MULTIPLE_BLOCK */
#define CMD32	(32)		/* ERASE_ER_BLK_START */
#define CMD33	(33)		/* ERASE_ER_BLK_END */
#define CMD38	(38)		/* ERASE */
#define CMD55	(55)		/* APP_CMD */
#define CMD58	(58)		/* READ_OCR */

/* MMC card type flags (MMC_GET_TYPE) */
#define CT_MMC		0x01		/* MMC ver 3 */
#define CT_SD1		0x02		/* SD ver 1 */
#define CT_SD2		0x04		/* SD ver 2 */
#define CT_SDC		(CT_SD1|CT_SD2)	/* SD */
#define CT_BLOCK	0x08		/* Block addressing */

static volatile
DSTATUS Stat = STA_NOINIT;	/* Physical drive status */


static
BYTE CardType;			/* Card type flags */

uint32_t spiTimerTickStart;
uint32_t spiTimerTickDelay;

void SPI_Timer_On(uint32_t waitTicks) {
    spiTimerTickStart = HAL_GetTick();
    spiTimerTickDelay = waitTicks;
}

uint8_t SPI_Timer_Status() {
    return ((HAL_GetTick() - spiTimerTickStart) < spiTimerTickDelay);
}

/*-----------------------------------------------------------------------*/
/* SPI controls (Platform dependent)                                     */
/*-----------------------------------------------------------------------*/

/* Exchange a byte */
static
BYTE xchg_spi (
	BYTE dat	/* Data to send */
)
{
	BYTE rxDat;
    HAL_SPI_TransmitReceive(&SD_SPI_HANDLE, &dat, &rxDat, 1, 50);
    return rxDat;
}


/* Receive multiple byte */
static
void rcvr_spi_multi (
	BYTE *buff,		/* Pointer to data buffer */
	UINT btr		/* Number of bytes to receive (even number) */
)
{
	for(UINT i=0; i<btr; i++) {
		*(buff+i) = xchg_spi(0xFF);
	}
}


#if _USE_WRITE
/* Send multiple byte */
static
void xmit_spi_multi (
	const BYTE *buff,	/* Pointer to the data */
	UINT btx			/* Number of bytes to send (even number) */
)
{
	HAL_SPI_Transmit(&SD_SPI_HANDLE, buff, btx, HAL_MAX_DELAY);
}
#endif


/*-----------------------------------------------------------------------*/
/* Wait for card ready                                                   */
/*-----------------------------------------------------------------------*/

// static
// int wait_ready (	/* 1:Ready, 0:Timeout */
// 	UINT wt			/* Timeout [ms] */
// )
// {
// 	BYTE d;
// 	//wait_ready needs its own timer, unfortunately, so it can't use the
// 	//spi_timer functions
// 	uint32_t waitSpiTimerTickStart;
// 	uint32_t waitSpiTimerTickDelay;

// 	waitSpiTimerTickStart = HAL_GetTick();
// 	waitSpiTimerTickDelay = (uint32_t)wt;
// 	do {
// 		d = xchg_spi(0xFF);
// 		/* This loop takes a time. Insert rot_rdq() here for multitask envilonment. */
// 	} while (d != 0xFF && ((HAL_GetTick() - waitSpiTimerTickStart) < waitSpiTimerTickDelay));	/* Wait for card goes ready or timeout */

// 	return (d == 0xFF) ? 1 : 0;
// }

// hong
static int wait_ready (	/* 1:Ready, 0:Timeout */
    UINT wt			/* Timeout [ms] */
)
{
    BYTE d;
    uint32_t waitSpiTimerTickStart;
    uint32_t waitSpiTimerTickDelay;
    uint32_t elapsed_time;

    waitSpiTimerTickStart = HAL_GetTick();
    waitSpiTimerTickDelay = (uint32_t)wt;
    do {
        d = xchg_spi(0xFF);
        /* This loop takes a time. Insert rot_rdq() here for multitask envilonment. */
    } while (d != 0xFF && ((HAL_GetTick() - waitSpiTimerTickStart) < waitSpiTimerTickDelay));	/* Wait for card goes ready or timeout */

    elapsed_time = HAL_GetTick() - waitSpiTimerTickStart;
    if (d == 0xFF) {
        // Optionally log success, but can be too verbose:
        SendMsg(&huart2, "wait_ready: OK after %lu ms (limit %u ms)\r\n", elapsed_time, wt);
        return 1;
    } else {
        SendMsg(&huart2, "wait_ready: TIMEOUT after %lu ms (limit %u ms)\r\n", elapsed_time, wt); // DEBUG
        return 0;
    }
}



/*-----------------------------------------------------------------------*/
/* Despiselect card and release SPI                                         */
/*-----------------------------------------------------------------------*/

static
void despiselect (void)
{
	CS_HIGH();		/* Set CS# high */
	xchg_spi(0xFF);	/* Dummy clock (force DO hi-z for multiple slave SPI) */

}



/*-----------------------------------------------------------------------*/
/* Select card and wait for ready                                        */
/*-----------------------------------------------------------------------*/

// static
// int spiselect (void)	/* 1:OK, 0:Timeout */
// {
// 	CS_LOW();		/* Set CS# low */
// 	xchg_spi(0xFF);	/* Dummy clock (force DO enabled) */
// 	if (wait_ready(500)) return 1;	/* Wait for card ready */

// 	despiselect();
// 	return 0;	/* Timeout */
// }

// hong
static int spiselect (void)	/* 1:OK, 0:Timeout */
{
    CS_LOW();		/* Set CS# low */
    xchg_spi(0xFF);	/* Dummy clock (force DO enabled) */
    // SendMsg(&huart2, "spiselect: calling wait_ready(500)\r\n"); // Can be too verbose
    if (wait_ready(5000)) { // Original timeout
        // SendMsg(&huart2, "spiselect: wait_ready OK\r\n"); // Can be too verbose
        return 1;	/* Wait for card ready */
    }

    SendMsg(&huart2, "spiselect: wait_ready(500) FAILED\r\n"); // DEBUG
    despiselect();
    return 0;	/* Timeout */
}


/*-----------------------------------------------------------------------*/
/* Receive a data packet from the MMC                                    */
/*-----------------------------------------------------------------------*/

static
int rcvr_datablock (	/* 1:OK, 0:Error */
	BYTE *buff,			/* Data buffer */
	UINT btr			/* Data block length (byte) */
)
{
	BYTE token;


	SPI_Timer_On(200);
	do {							/* Wait for DataStart token in timeout of 200ms */
		token = xchg_spi(0xFF);
		/* This loop will take a time. Insert rot_rdq() here for multitask envilonment. */
	} while ((token == 0xFF) && SPI_Timer_Status());
	if(token != 0xFE) return 0;		/* Function fails if invalid DataStart token or timeout */

	rcvr_spi_multi(buff, btr);		/* Store trailing data to the buffer */
	xchg_spi(0xFF); xchg_spi(0xFF);			/* Discard CRC */

	return 1;						/* Function succeeded */
}



/*-----------------------------------------------------------------------*/
/* Send a data packet to the MMC                                         */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
static
int xmit_datablock (	/* 1:OK, 0:Failed */
	const BYTE *buff,	/* Ponter to 512 byte data to be sent */
	BYTE token			/* Token */
)
{
	BYTE resp;


	if (!wait_ready(500)) return 0;		/* Wait for card ready */

	xchg_spi(token);					/* Send token */
	if (token != 0xFD) {				/* Send data if token is other than StopTran */
		xmit_spi_multi(buff, 512);		/* Data */
		xchg_spi(0xFF); xchg_spi(0xFF);	/* Dummy CRC */

		resp = xchg_spi(0xFF);				/* Receive data resp */
		if ((resp & 0x1F) != 0x05) return 0;	/* Function fails if the data packet was not accepted */
	}
	return 1;
}
#endif


/*-----------------------------------------------------------------------*/
/* Send a command packet to the MMC                                      */
/*-----------------------------------------------------------------------*/

static
BYTE send_cmd (		/* Return value: R1 resp (bit7==1:Failed to send) */
	BYTE cmd,		/* Command index */
	DWORD arg		/* Argument */
)
{
	BYTE n, res;


	if (cmd & 0x80) {	/* Send a CMD55 prior to ACMD<n> */
		cmd &= 0x7F;
		res = send_cmd(CMD55, 0);
		if (res > 1) return res;
	}

	/* Select the card and wait for ready except to stop multiple block read */
	if (cmd != CMD12) {
		despiselect();
		if (!spiselect()) return 0xFF;
	}

	/* Send command packet */
	xchg_spi(0x40 | cmd);				/* Start + command index */
	xchg_spi((BYTE)(arg >> 24));		/* Argument[31..24] */
	xchg_spi((BYTE)(arg >> 16));		/* Argument[23..16] */
	xchg_spi((BYTE)(arg >> 8));			/* Argument[15..8] */
	xchg_spi((BYTE)arg);				/* Argument[7..0] */
	n = 0x01;							/* Dummy CRC + Stop */
	if (cmd == CMD0) n = 0x95;			/* Valid CRC for CMD0(0) */
	if (cmd == CMD8) n = 0x87;			/* Valid CRC for CMD8(0x1AA) */
	xchg_spi(n);

	/* Receive command resp */
	if (cmd == CMD12) xchg_spi(0xFF);	/* Diacard following one byte when CMD12 */
	n = 10;								/* Wait for response (10 bytes max) */
	do {
		res = xchg_spi(0xFF);
	} while ((res & 0x80) && --n);

	return res;							/* Return received response */
}


/*--------------------------------------------------------------------------

   Public FatFs Functions (wrapped in user_diskio.c)

---------------------------------------------------------------------------*/

//The following functions are defined as inline because they aren't the functions that
//are passed to FatFs - they are wrapped by autogenerated (non-inline) cubemx template
//code.
//If you do not wish to use cubemx, remove the "inline" from these functions here
//and in the associated .h


/*-----------------------------------------------------------------------*/
/* Initialize disk drive                                                 */
/*-----------------------------------------------------------------------*/

// inline DSTATUS USER_SPI_initialize (
// 	BYTE drv		/* Physical drive number (0) */
// )
// {
// 	BYTE n, cmd, ty, ocr[4];

// 	if (drv != 0) return STA_NOINIT;		/* Supports only drive 0 */
// 	//assume SPI already init init_spi();	/* Initialize SPI */

// 	if (Stat & STA_NODISK) return Stat;	/* Is card existing in the soket? */

// 	FCLK_SLOW();
// 	for (n = 10; n; n--) xchg_spi(0xFF);	/* Send 80 dummy clocks */

// 	ty = 0;
// 	if (send_cmd(CMD0, 0) == 1) {			/* Put the card SPI/Idle state */
// 		SPI_Timer_On(1000);					/* Initialization timeout = 1 sec */
// 		if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
// 			for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);	/* Get 32 bit return value of R7 resp */
// 			if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* Is the card supports vcc of 2.7-3.6V? */
// 				while (SPI_Timer_Status() && send_cmd(ACMD41, 1UL << 30)) ;	/* Wait for end of initialization with ACMD41(HCS) */
// 				SendMsg(&huart2, "while (SPI_Timer_Status() && send_cmd(ACMD41, 1UL << 30))\r\n"); // DEBUG
// 				if (SPI_Timer_Status() && send_cmd(CMD58, 0) == 0) {		/* Check CCS bit in the OCR */
// 					for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);
// 					ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* Card id SDv2 */
// 				} else {
// 					SendMsg(&huart2, "else while (SPI_Timer_Status() && send_cmd(ACMD41, 1UL << 30))\r\n"); // DEBUG
// 				}
// 			}
// 		} else {	/* Not SDv2 card */
// 			if (send_cmd(ACMD41, 0) <= 1) 	{	/* SDv1 or MMC? */
// 				ty = CT_SD1; cmd = ACMD41;	/* SDv1 (ACMD41(0)) */
// 			} else {
// 				ty = CT_MMC; cmd = CMD1;	/* MMCv3 (CMD1(0)) */
// 			}
// 			while (SPI_Timer_Status() && send_cmd(cmd, 0)) ;		/* Wait for end of initialization */
// 			if (!SPI_Timer_Status() || send_cmd(CMD16, 512) != 0)	/* Set block length: 512 */
// 				ty = 0;
// 		}
// 	}
// 	CardType = ty;	/* Card type */
// 	despiselect();

// 	if (ty) {			/* OK */
// 		FCLK_FAST();			/* Set fast clock */
// 		Stat &= ~STA_NOINIT;	/* Clear STA_NOINIT flag */
// 	} else {			/* Failed */
// 		Stat = STA_NOINIT;
// 	}

// 	return Stat;
// }

// hong

inline DSTATUS USER_SPI_initialize (
    BYTE drv /* Physical drive number (0) */
)
{
    BYTE n, ty, cmd, ocr[4];

    if(drv) return STA_NOINIT;			/* Supports only drive 0 */
    if(Stat & STA_NODISK) return Stat;	/* No card in the socket */

    FCLK_SLOW();
    for (n = 10; n; n--) xchg_spi(0xFF);	/* Send 80 dummy clocks */

    ty = 0;
    SendMsg(&huart2, "CMD0 sent...\r\n"); // DEBUG
    if (send_cmd(CMD0, 0) == 1) {			/* Put the card SPI/Idle state */
        SendMsg(&huart2, "CMD0 OK (R1=0x01)\r\n"); // DEBUG
        SPI_Timer_On(3000);					/* Initialization timeout = 1 sec */
        SendMsg(&huart2, "CMD8 sent...\r\n"); // DEBUG
        if (send_cmd(CMD8, 0x1AA) == 1) {	/* SDv2? */
            SendMsg(&huart2, "CMD8 OK (R1=0x01)\r\n"); // DEBUG
            for (n = 0; n < 4; n++) ocr[n] = xchg_spi(0xFF);	/* Get 32 bit return value of R7 resp */
            SendMsg(&huart2, "CMD8 R7: %02X %02X %02X %02X\r\n", ocr[0], ocr[1], ocr[2], ocr[3]); // DEBUG
            if (ocr[2] == 0x01 && ocr[3] == 0xAA) {				/* Is the card supports vcc of 2.7-3.6V? */
                SendMsg(&huart2, "CMD8 Voltage OK\r\n"); // DEBUG
                BYTE acmd41_r1 = 0xFF; // Initialize with a non-ready state
                SendMsg(&huart2, "Starting ACMD41 polling. Timeout: %lu ms\r\n", spiTimerTickDelay); // DEBUG
                while (SPI_Timer_Status()) {
                    acmd41_r1 = send_cmd(ACMD41, 1UL << 30); /* Send ACMD41 with HCS bit */
                    SendMsg(&huart2, "ACMD41 attempt, R1=0x%02X\r\n", acmd41_r1); // DEBUG
                    if (acmd41_r1 == 0x00 || acmd41_r1 == 0xFF) break; /* Initialization completed */
                    if (acmd41_r1 != 0x01) {      /* Not busy (0x01) and not ready (0x00) -> error from send_cmd (e.g., 0xFF) */
                        SendMsg(&huart2, "ACMD41 poll: send_cmd returned error 0x%02X. Aborting poll.\r\n", acmd41_r1); // DEBUG
                        //break; 
                    }
                    /* Card is busy (R1=0x01), continue polling */
                    HAL_Delay(200); // Increased delay between polls
                }

                if ((acmd41_r1 == 0x00 || acmd41_r1 == 0xFF) && SPI_Timer_Status()) { /* Check if ready and timer has not expired */
                    SendMsg(&huart2, "ACMD41 OK (R1=0x00 || 0xFF)\r\n"); // DEBUG
                    BYTE cmd58_r1 = send_cmd(CMD58, 0);
					if (cmd58_r1 == 0 || cmd58_r1 == 0xFF) {		/* Check CCS bit in the OCR */
						SendMsg(&huart2, "CMD58 OK (R1=0x00 || 0xFF)\r\n"); // DEBUG
						SendMsg(&huart2, "Reading OCR bytes...\r\n"); // DEBUG
						for (n = 0; n < 4; n++) {
							ocr[n] = xchg_spi(0xFF); // Read one byte of OCR
							SendMsg(&huart2, "OCR byte %d: 0x%02X\r\n", n, ocr[n]); // DEBUG
						}
						SendMsg(&huart2, "OCR raw: %02X %02X %02X %02X\r\n", ocr[0], ocr[1], ocr[2], ocr[3]); // DEBUG
						ty = (ocr[0] & 0x40) ? CT_SD2 | CT_BLOCK : CT_SD2;	/* Card id SDv2 */
						SendMsg(&huart2, "Determined ty: 0x%02X based on ocr[0]=0x%02X\r\n", ty, ocr[0]); // DEBUG
					} else {
                        SendMsg(&huart2, "CMD58 failed after successful ACMD41 (cmd58_r1 : %d)\r\n", cmd58_r1); // DEBUG
                        ty = 0;
                    }
                } else { /* ACMD41 timed out or failed with an error */
                    if (!SPI_Timer_Status()) {
                        SendMsg(&huart2, "ACMD41 timed out (main timer %lu ms expired). Last R1=0x%02X\r\n", spiTimerTickDelay, acmd41_r1); // DEBUG
                    } else { 
                         SendMsg(&huart2, "ACMD41 poll failed. Last R1=0x%02X (main timer OK)\r\n", acmd41_r1); // DEBUG
                    }
                    ty = 0; // Ensure ty is 0 if ACMD41 didn't complete successfully
                }
            } else {
                SendMsg(&huart2, "CMD8 Voltage mismatch\r\n"); // DEBUG
            }
        } else {	/* Not SDv2 card */
            SendMsg(&huart2, "CMD8 failed (not R1=0x01), could be SDv1/MMC\r\n"); // DEBUG
            if (send_cmd(ACMD41, 0) <= 1) 	{	/* SDv1 or MMC? */
                SendMsg(&huart2, "ACMD41 (for SDv1) sent\r\n"); // DEBUG
                ty = CT_SD1; cmd = ACMD41;	/* SDv1 (ACMD41(0)) */
            } else {
                SendMsg(&huart2, "CMD1 (for MMC) sent\r\n"); // DEBUG
                ty = CT_MMC; cmd = CMD1;	/* MMCv3 (CMD1(0)) */
            }
            while (SPI_Timer_Status() && send_cmd(cmd, 0)) { /* Wait for end of initialization */
                SendMsg(&huart2, "SDv1/MMC init pending...\r\n"); // DEBUG
                HAL_Delay(10); // Small delay
            }
            if (!SPI_Timer_Status() || send_cmd(CMD16, 512) != 0) {	/* Set block length: 512 */
                SendMsg(&huart2, "SDv1/MMC init timeout or CMD16 failed\r\n"); // DEBUG
                ty = 0;
            } else {
                SendMsg(&huart2, "SDv1/MMC CMD16 OK\r\n"); // DEBUG
            }
        }
    } else {
        SendMsg(&huart2, "CMD0 failed (not R1=0x01)\r\n"); // DEBUG
    }

    CardType = ty;
    despiselect();

    if (ty) {			/* OK */
        SendMsg(&huart2, "SD Card Initialized. Type: %02X\r\n", ty); // DEBUG
        FCLK_FAST();			/* Set fast clock */
        Stat &= ~STA_NOINIT;	/* Clear STA_NOINIT flag */
    } else {			/* Failed */
        SendMsg(&huart2, "SD Card Initialization FAILED. ty=0\r\n"); // DEBUG
        Stat |= STA_NOINIT;
    }

    return Stat;
}

/*-----------------------------------------------------------------------*/
/* Get disk status                                                       */
/*-----------------------------------------------------------------------*/

inline DSTATUS USER_SPI_status (
	BYTE drv		/* Physical drive number (0) */
)
{
	if (drv) return STA_NOINIT;		/* Supports only drive 0 */

	return Stat;	/* Return disk status */
}



/*-----------------------------------------------------------------------*/
/* Read sector(s)                                                        */
/*-----------------------------------------------------------------------*/

inline DRESULT USER_SPI_read (
	BYTE drv,		/* Physical drive number (0) */
	BYTE *buff,		/* Pointer to the data buffer to store read data */
	DWORD sector,	/* Start sector number (LBA) */
	UINT count		/* Number of sectors to read (1..128) */
)
{
	if (drv || !count) return RES_PARERR;		/* Check parameter */
	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check if drive is ready */

	if (!(CardType & CT_BLOCK)) sector *= 512;	/* LBA ot BA conversion (byte addressing cards) */

	if (count == 1) {	/* Single sector read */
		if ((send_cmd(CMD17, sector) == 0)	/* READ_SINGLE_BLOCK */
			&& rcvr_datablock(buff, 512)) {
			count = 0;
		}
	}
	else {				/* Multiple sector read */
		if (send_cmd(CMD18, sector) == 0) {	/* READ_MULTIPLE_BLOCK */
			do {
				if (!rcvr_datablock(buff, 512)) break;
				buff += 512;
			} while (--count);
			send_cmd(CMD12, 0);				/* STOP_TRANSMISSION */
		}
	}
	despiselect();

	return count ? RES_ERROR : RES_OK;	/* Return result */
}



/*-----------------------------------------------------------------------*/
/* Write sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
inline DRESULT USER_SPI_write (
	BYTE drv,			/* Physical drive number (0) */
	const BYTE *buff,	/* Ponter to the data to write */
	DWORD sector,		/* Start sector number (LBA) */
	UINT count			/* Number of sectors to write (1..128) */
)
{
	if (drv || !count) return RES_PARERR;		/* Check parameter */
	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check drive status */
	if (Stat & STA_PROTECT) return RES_WRPRT;	/* Check write protect */

	if (!(CardType & CT_BLOCK)) sector *= 512;	/* LBA ==> BA conversion (byte addressing cards) */

	if (count == 1) {	/* Single sector write */
		if ((send_cmd(CMD24, sector) == 0)	/* WRITE_BLOCK */
			&& xmit_datablock(buff, 0xFE)) {
			count = 0;
		}
	}
	else {				/* Multiple sector write */
		if (CardType & CT_SDC) send_cmd(ACMD23, count);	/* Predefine number of sectors */
		if (send_cmd(CMD25, sector) == 0) {	/* WRITE_MULTIPLE_BLOCK */
			do {
				if (!xmit_datablock(buff, 0xFC)) break;
				buff += 512;
			} while (--count);
			if (!xmit_datablock(0, 0xFD)) count = 1;	/* STOP_TRAN token */
		}
	}
	despiselect();

	return count ? RES_ERROR : RES_OK;	/* Return result */
}
#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous drive controls other than data read/write               */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
inline DRESULT USER_SPI_ioctl (
	BYTE drv,		/* Physical drive number (0) */
	BYTE cmd,		/* Control command code */
	void *buff		/* Pointer to the conrtol data */
)
{
	DRESULT res;
	BYTE n, csd[16];
	DWORD *dp, st, ed, csize;


	if (drv) return RES_PARERR;					/* Check parameter */
	if (Stat & STA_NOINIT) return RES_NOTRDY;	/* Check if drive is ready */

	res = RES_ERROR;

	switch (cmd) {
	case CTRL_SYNC :		/* Wait for end of internal write process of the drive */
		if (spiselect()) res = RES_OK;
		break;

	case GET_SECTOR_COUNT :	/* Get drive capacity in unit of sector (DWORD) */
		if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {
			if ((csd[0] >> 6) == 1) {	/* SDC ver 2.00 */
				csize = csd[9] + ((WORD)csd[8] << 8) + ((DWORD)(csd[7] & 63) << 16) + 1;
				*(DWORD*)buff = csize << 10;
			} else {					/* SDC ver 1.XX or MMC ver 3 */
				n = (csd[5] & 15) + ((csd[10] & 128) >> 7) + ((csd[9] & 3) << 1) + 2;
				csize = (csd[8] >> 6) + ((WORD)csd[7] << 2) + ((WORD)(csd[6] & 3) << 10) + 1;
				*(DWORD*)buff = csize << (n - 9);
			}
			res = RES_OK;
		}
		break;

	case GET_BLOCK_SIZE :	/* Get erase block size in unit of sector (DWORD) */
		if (CardType & CT_SD2) {	/* SDC ver 2.00 */
			if (send_cmd(ACMD13, 0) == 0) {	/* Read SD status */
				xchg_spi(0xFF);
				if (rcvr_datablock(csd, 16)) {				/* Read partial block */
					for (n = 64 - 16; n; n--) xchg_spi(0xFF);	/* Purge trailing data */
					*(DWORD*)buff = 16UL << (csd[10] >> 4);
					res = RES_OK;
				}
			}
		} else {					/* SDC ver 1.XX or MMC */
			if ((send_cmd(CMD9, 0) == 0) && rcvr_datablock(csd, 16)) {	/* Read CSD */
				if (CardType & CT_SD1) {	/* SDC ver 1.XX */
					*(DWORD*)buff = (((csd[10] & 63) << 1) + ((WORD)(csd[11] & 128) >> 7) + 1) << ((csd[13] >> 6) - 1);
				} else {					/* MMC */
					*(DWORD*)buff = ((WORD)((csd[10] & 124) >> 2) + 1) * (((csd[11] & 3) << 3) + ((csd[11] & 224) >> 5) + 1);
				}
				res = RES_OK;
			}
		}
		break;

	case CTRL_TRIM :	/* Erase a block of sectors (used when _USE_ERASE == 1) */
		if (!(CardType & CT_SDC)) break;				/* Check if the card is SDC */
		if (USER_SPI_ioctl(drv, MMC_GET_CSD, csd)) break;	/* Get CSD */
		if (!(csd[0] >> 6) && !(csd[10] & 0x40)) break;	/* Check if sector erase can be applied to the card */
		dp = buff; st = dp[0]; ed = dp[1];				/* Load sector block */
		if (!(CardType & CT_BLOCK)) {
			st *= 512; ed *= 512;
		}
		if (send_cmd(CMD32, st) == 0 && send_cmd(CMD33, ed) == 0 && send_cmd(CMD38, 0) == 0 && wait_ready(30000)) {	/* Erase sector block */
			res = RES_OK;	/* FatFs does not check result of this command */
		}
		break;

	default:
		res = RES_PARERR;
	}

	despiselect();

	return res;
}
#endif
