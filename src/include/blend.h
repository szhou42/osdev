#ifndef BLEND_H
#define BLEND_H
#include <system.h>


#define GET_ALPHA(color) ((color >> 24) & 0x000000FF)
#define GET_RED(color)   ((color >> 16)   & 0x000000FF)
#define GET_GREEN(color) ((color >> 8)  & 0x000000FF)
#define GET_BLUE(color)  ((color >> 0)   & 0X000000FF)


#define SET_ALPHA(color, alpha) ( ((color << 8) >> 8) | ((alpha << 24) & 0xff000000))

uint32_t blend_colors(uint32_t color1, uint32_t color2);

#endif
