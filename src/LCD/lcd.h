#ifndef _LCD
#define _LCD

#include "altera_avalon_pio_regs.h"
#include <altera_avalon_sgdma.h>
#include <altera_avalon_sgdma_descriptor.h>
#include <altera_avalon_sgdma_regs.h>

#define BACKLIGHT_PIN 	0x40
#define DISP_PIN		0x80

#define lcd_backlight_on()	IOWR_ALTERA_AVALON_PIO_SET_BITS(LED_PIO_BASE, BACKLIGHT_PIN)
#define lcd_backlight_off()	IOWR_ALTERA_AVALON_PIO_CLEAR_BITS(LED_PIO_BASE, BACKLIGHT_PIN)

#define lcd_on()			IOWR_ALTERA_AVALON_PIO_SET_BITS(LED_PIO_BASE, BACKLIGHT_PIN | DISP_PIN)
#define lcd_off()			IOWR_ALTERA_AVALON_PIO_CLEAR_BITS(LED_PIO_BASE, BACKLIGHT_PIN | DISP_PIN)

extern alt_sgdma_descriptor dmaDescA[8];
extern alt_sgdma_descriptor dmaDescEND;
extern alt_sgdma_descriptor dmaDescB[8];
extern alt_sgdma_descriptor dmaDescBEND;

extern alt_u32* frameBufferA;
extern alt_u32* frameBufferB;

extern int active_buffer;


void my_dma_callback(void *data);

void init_and_start_framebuffer(alt_sgdma_dev *dma);

#endif
