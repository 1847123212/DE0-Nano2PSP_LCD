/*----------------------------------------------------------------------*/
/* FatFs sample project for generic microcontrollers (C)ChaN, 2012      */
/*----------------------------------------------------------------------*/

#include <stdio.h>
#include <string.h>

#include "altera_avalon_pio_regs.h"
#include <altera_avalon_sgdma.h>
#include <altera_avalon_sgdma_descriptor.h>
#include <altera_avalon_sgdma_regs.h>

#include "ff/ff.h"

// some useful structs ----------------------------
//

// a pixel in 32 bit
typedef union
{
    alt_u32 color32;
    struct allColor {
        alt_u8 b;
        alt_u8 g;
        alt_u8 r;
        alt_u8 blank;
    }color8;
}Color;

// a particle, just for funsies
typedef struct
{
    int x, y;
    int vx, vy;
}Particle;


FATFS Fatfs;		/* File system object */
FIL Fil;			/* File object */
BYTE Buff[1];		/* File read buffer */

// global variables  -----------------------------
//

// DMA descriptors
alt_sgdma_descriptor dmaDescA[8];
alt_sgdma_descriptor dmaDescEND;
alt_sgdma_descriptor dmaDescB[8];
alt_sgdma_descriptor dmaDescBEND;


// we locate our first framebuffer at the beginning of the SDRAM
alt_u32* frameBufferA = (alt_u32*)SDRAM_BASE;
alt_u32* frameBufferB = (alt_u32*)SDRAM_BASE+0x7F800;

// Active framebuffer
int active = 0;		// if 0 A active else B active


// DMA  -------------------------------------
//

// The InterruptService Routine (actually a callback function called by the ISR)
//
void my_dma_callback(void *data)
{
    // reset the OWNED_BY_HW bit in the descriptors to reuse the chain
    int i;

    if (active == 0) {
    	for(i = 0; i < 8;++i) {
    		dmaDescA[i].control |= 1<<ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_OFST;
    	}
        // trigger another transfer all over again
        alt_avalon_sgdma_do_async_transfer((alt_sgdma_dev*)data, dmaDescA);
//        active = 1;
    } else {
    	for(i = 0; i < 8;++i) {
    	    dmaDescB[i].control |= 1<<ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_OFST;
    	}
        // trigger another transfer all over again
         alt_avalon_sgdma_do_async_transfer((alt_sgdma_dev*)data, dmaDescB);
//         active = 0;
    }
}


// this subroutine initializes a chain of descriptors, registers the
// interrupt service routine and starts the first asynchronous transfer
//
void init_and_start_framebuffer(alt_sgdma_dev *dma)
{
    // 480*272 lines * 4 bytes = 522240 bytes
    //   65532 (0xfffc) bytes * 7 = 458724
    //  +63516 (0xf81c) bytes

    // frame buffer A
    alt_u8* buff = (alt_u8*)frameBufferA;
    int i;
    for(i = 0; i < 8; ++i) {
        alt_u16 size = (i<7)?0xfffc:0xf81c;
	alt_avalon_sgdma_construct_mem_to_stream_desc(
	    &dmaDescA[i],
	    (i<7) ? (&dmaDescA[i+1]) : &dmaDescEND,
	    (alt_u32*)buff,
	    size, 0, i==0, i==7, 0);
	    buff+= size;
	}

    // frame buffer B
    buff = (alt_u8*)frameBufferB;
    for(i = 0; i < 8; ++i) {
        alt_u16 size = (i<7)?0xfffc:0xf81c;
	alt_avalon_sgdma_construct_mem_to_stream_desc(
	    &dmaDescB[i],
	    (i<7) ? (&dmaDescB[i+1]) : &dmaDescBEND,
	    (alt_u32*)buff,
	    size, 0, i==0, i==7, 0);
	    buff+= size;
	}

	alt_avalon_sgdma_register_callback(
            dma, my_dma_callback,
            ALTERA_AVALON_SGDMA_CONTROL_IE_CHAIN_COMPLETED_MSK
            |ALTERA_AVALON_SGDMA_CONTROL_IE_GLOBAL_MSK,
            (void*)dma);

	alt_avalon_sgdma_do_async_transfer(dma, dmaDescA);
}

// basic drawing routines --------------------------------------
//


// draw a pixel
inline void setPix(const int x, const int y, const Color col, alt_u32* buffer)
{
    buffer[x + y * 480] = col.color32;
}

// bresenham line drawing
void line(alt_u32* buffer, int x0, int y0, int x1, int y1, Color color) {

    int dx = abs(x1-x0), sx = x0<x1 ? 1 : -1;
    int dy = abs(y1-y0), sy = y0<y1 ? 1 : -1;
    int err = (dx>dy ? dx : -dy)/2, e2;

    for(;;){
        setPix(x0, y0, color, buffer);
	if (x0==x1 && y0==y1) break;
	e2 = err;
	if (e2 >-dx) { err -= dy; x0 += sx; }
	if (e2 < dy) { err += dx; y0 += sy; }
    }
}

// useful to fill parts of the screen without blocking the SDRAM for too long
// use this instead of memset when doing frame buffer fills
void nonburst_memset(alt_u32* trg, alt_u32 val, alt_u32 size)
{
const int chunkSize = 8;// <-size of burst, lower this if the display gets corrupted.
alt_u32 s;
//int i;
    while(size) {
	s = (size>chunkSize)?chunkSize:size;
	memset(trg, val, s*4);
        trg+=s;
        //for(i=0;i<30;i++) {}
	size-=s;
    }
}

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
    IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, 0);
    for(i=0;i<5000000;i++){}

    // switch on the backlight
    //
    IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, 64);

    // initialize the DMA and get a device handle
    //
    alt_sgdma_dev *dma = alt_avalon_sgdma_open("/dev/sgdma");
    printf("open dma returned %ld\n", (alt_u32)dma);
    printf("framebuffer 1 at %lx\n", (alt_u32)frameBufferA);


    // assert the DISP signal
    //
    IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, 128+64);

    init_and_start_framebuffer(dma);


	f_mount(0, &Fatfs);		/* Register volume work area (never fails) */

//	printf("\nOpen an existing file (message.txt).\n");
//	rc = f_open(&Fil, "christina.psp", FA_READ);
//	if (rc) die(rc);
//
//	printf("\nType the file content.\n");
//	for (;;) {
//		rc = f_read(&Fil, frameBufferA, f_size(&Fil), &br);	/* Read a chunk of file */
//		if (rc || !br) break;			/* Error or end of file */
//		//for (i = 0; i < br; i++)		/* Type the data */
//		//	putchar(Buff[i]);
//	}
//	if (rc) die(rc);
//
//	printf("\nClose the file.\n");
//	rc = f_close(&Fil);
//	if (rc) die(rc);
//
//
//	printf("\nOpen an existing file (message.txt).\n");
//	rc = f_open(&Fil, "large2.psp", FA_READ);
//	if (rc) die(rc);
//
//	printf("\nType the file content.\n");
//	for (;;) {
//		rc = f_read(&Fil, frameBufferA, f_size(&Fil), &br);	/* Read a chunk of file */
//		if (rc || !br) break;			/* Error or end of file */
//		//for (i = 0; i < br; i++)		/* Type the data */
//		//	putchar(Buff[i]);
//	}
//	if (rc) die(rc);
//
//	printf("\nClose the file.\n");
//	rc = f_close(&Fil);
//	if (rc) die(rc);

//	printf("\nCreate a new file (hello.txt).\n");
//	rc = f_open(&Fil, "HELLO.TXT", FA_WRITE | FA_CREATE_ALWAYS);
//	if (rc) die(rc);
//
//	printf("\nWrite a text data. (Hello world!)\n");
//	rc = f_write(&Fil, "Hello world!\r\n", 14, &bw);
//	if (rc) die(rc);
//	printf("%u bytes written.\n", bw);
//
//	printf("\nClose the file.\n");
//	rc = f_close(&Fil);
//	if (rc) die(rc);

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
			if (active == 0) {
				rc = f_read(&Fil, frameBufferB, f_size(&Fil), &br);	/* Read a chunk of file */
				if (rc || !br) {
					active = 1;
					break;			/* Error or end of file */
				}
			} else {
				rc = f_read(&Fil, frameBufferA, f_size(&Fil), &br);	/* Read a chunk of file */
				if (rc || !br) {
					active = 0;
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
	    		if(active == 0) {
	    			active = 1;
	    			nonburst_memset(frameBufferA, col.color32, 130560);
	    		} else {
	    			active = 0;
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
	    	if (active == 0) {
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
