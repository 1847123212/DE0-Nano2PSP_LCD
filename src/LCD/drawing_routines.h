#ifndef _DRAWING_ROUTINES
#define _DRAWING_ROUTINES

#include <alt_types.h>

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


inline void setPix(const int x, const int y, const Color col, alt_u32* buffer);
void line(alt_u32* buffer, int x0, int y0, int x1, int y1, Color color);
void nonburst_memset(alt_u32* trg, alt_u32 val, alt_u32 size);


#endif
