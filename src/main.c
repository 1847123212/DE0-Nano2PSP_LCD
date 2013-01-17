/*----------------------------------------------------------------------*/
/* Digital picture frame project, Morten Jensen 2013      		*/
/* I have used code for varios places for this project.			*/
/* This is mainly the FAT lib and some code for the display.		*/
/* The FAT lib can be found at: 					*/
/* http://elm-chan.org/fsw/ff/00index_e.html				*/
/* The inspiration for the display code come from: 			*/
/* http://sites.google.com/site/fpgaandco/				*/
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include <altera_avalon_pio_regs.h>
#include <altera_avalon_sgdma.h>
#include <altera_avalon_sgdma_descriptor.h>
#include <altera_avalon_sgdma_regs.h>

#include <sys/alt_alarm.h>

#include "LCD/lcd.h"
#include "LCD/drawing_routines.h"
#include "libfatfs/core/ff.h"


FATFS Fatfs;		/* File system object */
FIL Fil;			/* File object */
char Buff[10];		/* File read buffer */

/*=========================================================================*/
/*  DEFINE: Prototypes                                                     */
/*=========================================================================*/

/*=========================================================================*/
/*  DEFINE: Definition of all local Data                                   */
/*=========================================================================*/
static alt_alarm alarm;
static unsigned long Systick = 0;
static volatile unsigned short Timer;   /* 1000Hz increment timer */

/*=========================================================================*/
/*  DEFINE: Definition of all local Procedures                             */
/*=========================================================================*/

/***************************************************************************/
/*  TimerFunction                                                          */
/*                                                                         */
/*  This timer function will provide a 10ms timer and                      */
/*  call ffs_DiskIOTimerproc.                                              */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static alt_u32 TimerFunction (void *context)
{
   static unsigned short wTimer10ms = 0;

   (void)context;

   Systick++;
   wTimer10ms++;
   Timer++; /* Performance counter for this module */

   if (wTimer10ms == 10)
   {
      wTimer10ms = 0;
      ffs_DiskIOTimerproc();  /* Drive timer procedure of low level disk I/O module */
   }

   return(1);
} /* TimerFunction */

/***************************************************************************/
/*  IoInit                                                                 */
/*                                                                         */
/*  Init the hardware like GPIO, UART, and more...                         */
/*                                                                         */
/*  In    : none                                                           */
/*  Out   : none                                                           */
/*  Return: none                                                           */
/***************************************************************************/
static void IoInit(void)
{
   //uart0_init(115200);

   /* Init diskio interface */
   ffs_DiskIOInit();

   /* Init timer system */
   alt_alarm_start(&alarm, 1, &TimerFunction, NULL);

} /* IoInit */

void die (		/* Stop with dying message */
	FRESULT rc	/* FatFs return value */
)
{
	printf("Failed with rc=%u.\n", rc);
	for (;;) ;
}

int main (void)
{
	FRESULT rc;				/* Result code */
	DIR dir;				/* Directory object */
	FILINFO fno;			/* File information object */
	unsigned long br, i;
	alt_u32* active_buff;

 	printf("System started\r\n");
 	lcd_off();
    for(i=0;i<5000000;i++){}

    alt_sgdma_dev *dma = alt_avalon_sgdma_open("/dev/sgdma");
    printf("open dma returned %ld\n", (alt_u32)dma);
    printf("framebuffer 1 at %lx\n", (alt_u32)frameBufferA);
    printf("framebuffer 2 at %lx\n", (alt_u32)frameBufferB);

    active_buff = frameBufferA;
    //printf("framebuffer 1 at %lx\n", (alt_u32)active_buff);

    printf("Turning LCD on \r\n");
    lcd_on();
    printf("Starting DMA\r\n");
    init_and_start_framebuffer(dma);

	IoInit();

    Color col2;
    col2.color32 = 0x00ff0000;

    memset(frameBufferA, col2.color32,  522240/4);

	f_mount(0, &Fatfs);		/* Register volume work area (never fails) */

	for (;;) {
	printf("\nOpen root directory.\n");
	rc = f_opendir(&dir, "");
	if (rc) die(rc);

	printf("\nDirectory listing...\n");
	for (;;) {
		rc = f_readdir(&dir, &fno);		/* Read a directory item */
		if (rc || !fno.fname[0]) break;	/* Error or end of dir */
		if (fno.fattrib & AM_DIR)
			printf("   <dir>  %s\n", fno.fname);
		else {
			printf("%8lu  %s\n", fno.fsize, fno.fname);
		//printf("\nOpen an existing file.\n");
		rc = f_open(&Fil, fno.fname, FA_READ);
		if (rc) die(rc);

		if (active_buffer == 0) {
			active_buffer = 1;
			active_buff = frameBufferA;
		} else {
			active_buff = frameBufferB;
			active_buffer = 0;
		}

		for (;;) {
			rc = f_read(&Fil, (alt_u32*)active_buff, f_size(&Fil), &br);	/* Read a chunk of file */
			if (rc || !br)
				break;			/* Error or end of file */
		}
		if (rc) die(rc);
		rc = f_close(&Fil);
		if (rc) die(rc);
		}

	}
	if (rc) die(rc);
	}
}



///*---------------------------------------------------------*/
///* User Provided Timer Function for FatFs module           */
///*---------------------------------------------------------*/
//
//DWORD get_fattime (void)
//{
//	return	  ((DWORD)(2012 - 1980) << 25)	/* Year = 2012 */
//			| ((DWORD)1 << 21)				/* Month = 1 */
//			| ((DWORD)1 << 16)				/* Day_m = 1*/
//			| ((DWORD)0 << 11)				/* Hour = 0 */
//			| ((DWORD)0 << 5)				/* Min = 0 */
//			| ((DWORD)0 >> 1);				/* Sec = 0 */
//}
