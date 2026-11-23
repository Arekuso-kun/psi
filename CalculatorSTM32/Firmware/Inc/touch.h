#ifndef MC_TOUCH_H
#define MC_TOUCH_H
#include <stdint.h>
typedef struct { uint16_t x,y;uint8_t down; } McTouch;
int mc_touch_init(void);
int mc_touch_poll(uint32_t now,McTouch *sample);
int mc_touch_available(void);
#endif
