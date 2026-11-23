#ifndef MC_JOYSTICK_H
#define MC_JOYSTICK_H
#include "input.h"
typedef struct { uint8_t stable,candidate,last_direction;uint32_t since,repeat_at; } McJoystick;
void mc_joystick_init(McJoystick *j);
void mc_joystick_poll(McJoystick *j,McInput *input,uint8_t raw,uint32_t now);
#endif
