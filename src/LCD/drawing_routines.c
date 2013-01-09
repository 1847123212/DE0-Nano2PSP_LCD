#include <stdlib.h>
#include <string.h>

#include "drawing_routines.h"

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
