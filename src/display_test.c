#include <stdio.h>

#include "altera_avalon_pio_regs.h"
#include <altera_avalon_sgdma.h>
#include <altera_avalon_sgdma_descriptor.h>
#include <altera_avalon_sgdma_regs.h>


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



// global variables  -----------------------------
//

// DMA descriptors
alt_sgdma_descriptor dmaDescA[8];
alt_sgdma_descriptor dmaDescEND;


// we locate our first framebuffer at the beginning of the SDRAM
alt_u32* frameBufferA = (alt_u32*)SDRAM_BASE;




// DMA  -------------------------------------
//

// The InterruptService Routine (actually a callback function called by the ISR)
//
void my_dma_callback(void *data)
{
    // reset the OWNED_BY_HW bit in the descriptors to reuse the chain
    int i;
    for(i = 0; i < 8;++i)
        dmaDescA[i].control |=
            1<<ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_OFST;

    // trigger another transfer all over again
    alt_avalon_sgdma_do_async_transfer((alt_sgdma_dev*)data, dmaDescA);
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


// fill the screen with a fractal (quite slow)
void MandelBrot(alt_u32* buffer)
{

    alt_32 x,xx,y,cx,cy;
    int iteration,hx,hy;
    int itermax = 64;		/* how many iterations to do	*/
    int px, py;
    Color ramp[67];

    memset(ramp, 0, sizeof(ramp));

    // lets prepare some pretty colors
    for(x = 0; x < 32; x++) {
        ramp[x].color8.r = x*8;
	ramp[x].color8.g = 255- x*8;
	ramp[x].color8.b = 0;
    }
    for(x = 0; x < 32; x++) {
        ramp[32+x].color8.r = 255 - x*8;
	ramp[32+x].color8.g = 0;
	ramp[32+x].color8.b = x*8;
    }

    py = 0;
    for (hy=-1500;py<272; hy+=6, ++py)  {
        cy = hy;
	px=0;
	for (hx=-2200;px<480; hx+=6, ++px)  {
	    cx = hx;
	    x = y = 0;
	    for (iteration=0; iteration < itermax; iteration++)  {
	        x/=8;y/=8;
		xx = x*x/16-y*y/16+cx;
		y = 2*x*y/16+cy;
		x = xx;
		if ((x/32)*(x/32)+(y/32)*(y/32)>1000000) break;
	    }
	    setPix(px, py, ramp[iteration], buffer);
	}
    }
}

// useful to fill parts of the screen without blocking the SDRAM for too long
// use this instead of memset when doing frame buffer fills
void nonburst_memset(alt_u32* trg, alt_u32 val, alt_u32 size)
{
const int chunkSize = 12;// <-size of burst, lower this if the display gets corrupted.
alt_u32 s;
    while(size) {
	s = (size>chunkSize)?chunkSize:size;
	memset(trg, val, s*4);
        trg+=s;
	size-=s;
    }
}


int main()
{
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

    // now we actually have some time to draw something
    //
    Particle particles[2] = { {100,0, -1, -1}, {0, 120, -1, -1} };
    Color col;
    int count;
    int i;
    int x,y;
    int j = 0;
    int k = 255;
    int dir = 1;

    for(count=1;;count++) {

        // blink an led to see the program running
    	IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, (count & 0x0001)+128+64);

    	//MandelBrot(frameBufferA);
    	col.color8.r = 255;
    	col.color8.g = 255;
    	col.color8.b = 255;
    	if (count%5000 == 0)
    		nonburst_memset(frameBufferA, col.color32, 522240/4);
//    	for (x=0;x<480;x++) {
//    		for (y=0;y<272;y++) {
//    			setPix(x, y, col, frameBufferA);
//    		}
//    	}
    	for(i=0;i<=10000;i++) {};

    	if((j>=255) && (dir == 1)) {
    		dir = -1;
    	} else if((j <= 0) && (dir == -1)) {
    		dir = 1;
    	}

    	j += dir;
    	k += -1*dir;

    	// draw a line in a random color
    	col.color8.r = j;
    	col.color8.g = k;
    	col.color8.b = 50;
        line(frameBufferA, particles[0].x, particles[0].y,
                            particles[1].x, particles[1].y, col);

        // bounce the particles around
        int i;
        for(i = 0; i < 2; ++i) {
            if(particles[i].x == 0 || particles[i].x == 479) particles[i].vx *= -1;
            particles[i].x += particles[i].vx;
            if(particles[i].y == 0 || particles[i].y == 271) particles[i].vy *= -1;
            particles[i].y += particles[i].vy;
        }
    }

    return 0;
}
