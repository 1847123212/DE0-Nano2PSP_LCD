#include "lcd.h"

// DMA descriptors
alt_sgdma_descriptor dmaDescA[8];
alt_sgdma_descriptor dmaDescEND;
alt_sgdma_descriptor dmaDescB[8];
alt_sgdma_descriptor dmaDescBEND;


// we locate our first framebuffer at the beginning of the SDRAM
alt_u32* frameBufferA = (alt_u32*)SDRAM_BASE;
alt_u32* frameBufferB = (alt_u32*)SDRAM_BASE+0x7F800;

// Active framebuffer
int active_buffer = 0;		// if 0 A active else B active


// The InterruptService Routine (actually a callback function called by the ISR)
//
void my_dma_callback(void *data)
{
    // reset the OWNED_BY_HW bit in the descriptors to reuse the chain
    int i;

    if (active_buffer == 0) {
    	for(i = 0; i < 8;++i) {
    		dmaDescA[i].control |= 1<<ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_OFST;
    	}
        // trigger another transfer all over again
        alt_avalon_sgdma_do_async_transfer((alt_sgdma_dev*)data, dmaDescA);
    } else {
    	for(i = 0; i < 8;++i) {
    	    dmaDescB[i].control |= 1<<ALTERA_AVALON_SGDMA_DESCRIPTOR_CONTROL_OWNED_BY_HW_OFST;
    	}
        // trigger another transfer all over again
         alt_avalon_sgdma_do_async_transfer((alt_sgdma_dev*)data, dmaDescB);
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

//void lcd_backlight_on(void) {
//	IOWR_ALTERA_AVALON_PIO_DATA(LED_PIO_BASE, 64);
//}
