#ifndef MC_GRAPHICS_H
#define MC_GRAPHICS_H
#include <stdint.h>
void mc_gfx_buffer(uint16_t *pixels);
void mc_gfx_rect(int x,int y,int w,int h,uint16_t color);
void mc_gfx_text(int x,int y,const char *text,int scale,uint16_t color);
void mc_gfx_center(int y,const char *text,int scale,uint16_t color);
#endif
