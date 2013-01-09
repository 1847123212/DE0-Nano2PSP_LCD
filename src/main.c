/*----------------------------------------------------------------------*/
/* FatFs sample project for generic microcontrollers (C)ChaN, 2012      */
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "altera_avalon_pio_regs.h"
#include <altera_avalon_sgdma.h>
#include <altera_avalon_sgdma_descriptor.h>
#include <altera_avalon_sgdma_regs.h>

#include "LCD/lcd.h"
#include "LCD/drawing_routines.h"
#include "ff/ff.h"


FATFS Fatfs;		/* File system object */
FIL Fil;			/* File object */
BYTE Buff[1];		/* File read buffer */

void die (		/* Stop with dying message */
	FRESULT rc	/* FatFs return value */
)
{
	printf("Failed with rc=%u.\n", rc);
	for (;;) ;
}


/*-----------------------------------------------------------------------*/
/* Program Main                                                          */
/*-----------------------------------------------------------------------*/

int main (void)
{
	FRESULT rc;				/* Result code */
	DIR dir;				/* Directory object */
	FILINFO fno;			/* File information object */
	UINT bw, br, i;

	// Switch all off
    lcd_off();
    for(i=0;i<5000000;i++){}

    // switch on the backlight
    //

    // initialize the DMA and get a device handle
    //
    alt_sgdma_dev *dma = alt_avalon_sgdma_open("/dev/sgdma");
    printf("open dma returned %ld\n", (alt_u32)dma);
    printf("framebuffer 1 at %lx\n", (alt_u32)frameBufferA);
    init_and_start_framebuffer(dma);

    lcd_on();


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
		printf("\nOpen an existing file.\n");
		rc = f_open(&Fil, fno.fname, FA_READ);
		if (rc) die(rc);

		printf("\nType the file content.\n");
		for (;;) {
			if (active_buffer == 0) {
				rc = f_read(&Fil, frameBufferB, f_size(&Fil), &br);	/* Read a chunk of file */
				if (rc || !br) {
					active_buffer = 1;
					break;			/* Error or end of file */
				}
			} else {
				rc = f_read(&Fil, frameBufferA, f_size(&Fil), &br);	/* Read a chunk of file */
				if (rc || !br) {
					active_buffer = 0;
					break;			/* Error or end of file */
				}
			}
			//for (i = 0; i < br; i++)		/* Type the data */
			//	putchar(Buff[i]);
		}
		if (rc) die(rc);

		printf("\nClose the file.\n");
		rc = f_close(&Fil);
		if (rc) die(rc);
		}

	}
	if (rc) die(rc);
	}

	printf("\nTest completed.\n");
	for(i=0;i<=30000000;i++) {};
	   // now we actually have some time to draw something
	    //
	    Particle particles[2] = { {100,0, -1, -1}, {0, 120, -1, -1} };
	    Color col;
	    int count;
	    int x,y;
	    int j = 0;
	    int k = 255;
	    int dire = 1;

	    col.color32 = 0xffffffff;
	    nonburst_memset(frameBufferA, col.color32, 522240/4);

	    for(count=1;;count++) {

	        // blink an led to see the program running
	    	IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, (count & 0x0001)+128+64);

	    	//MandelBrot(frameBufferA);
	    	col.color8.r = 255;
	    	col.color8.g = 255;
	    	col.color8.b = 255;



	    	for(i=0;i<=3000;i++) {};

	    	if (count%5000 == 0) {
	    		if(active_buffer == 0) {
	    			active_buffer = 1;
	    			nonburst_memset(frameBufferA, col.color32, 130560);
	    		} else {
	    			active_buffer = 0;
	    			nonburst_memset(frameBufferB, col.color32, 130560);
	    		}
	    	}

	    	if((j>=255) && (dire == 1)) {
	    		dire = -1;
	    	} else if((j <= 0) && (dire == -1)) {
	    		dire = 1;
	    	}

	    	j += dire;
	    	k += -1*dire;

	    	// draw a line in a random color
	    	col.color8.r = j;
	    	col.color8.g = 0;
	    	col.color8.b = 50;
	    	if (active_buffer == 0) {
	    		line(frameBufferA, particles[0].x, particles[0].y,
	    				particles[1].x, particles[1].y, col);
	    	} else {
	      		line(frameBufferB, particles[0].x, particles[0].y,
	        				particles[1].x, particles[1].y, col);
	    	}

	        // bounce the particles around
	        int i;
	        for(i = 0; i < 2; ++i) {
	            if(particles[i].x == 0 || particles[i].x == 479) particles[i].vx *= -1;
	            particles[i].x += particles[i].vx;
	            if(particles[i].y == 0 || particles[i].y == 271) particles[i].vy *= -1;
	            particles[i].y += particles[i].vy;
	        }
	    }
}



/*---------------------------------------------------------*/
/* User Provided Timer Function for FatFs module           */
/*---------------------------------------------------------*/

DWORD get_fattime (void)
{
	return	  ((DWORD)(2012 - 1980) << 25)	/* Year = 2012 */
			| ((DWORD)1 << 21)				/* Month = 1 */
			| ((DWORD)1 << 16)				/* Day_m = 1*/
			| ((DWORD)0 << 11)				/* Hour = 0 */
			| ((DWORD)0 << 5)				/* Min = 0 */
			| ((DWORD)0 >> 1);				/* Sec = 0 */
}
