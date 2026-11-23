#ifndef MC_BOARD_CONFIG_H
#define MC_BOARD_CONFIG_H
#include "stm32f4xx_hal.h"
/* Custom wiring: REMOVE all five joystick-to-PG jumpers on Open429Z-D first.
 * B(left)->PG2, C(right)->PG3, D(up)->PE2, A(down)->PE3, PRESS->PE4.
 * The original PG6/PG10/PG11/PG12 connections conflict with the LCD. */
#define MC_JOY_L_PORT GPIOG
#define MC_JOY_L_PIN GPIO_PIN_2
#define MC_JOY_R_PORT GPIOG
#define MC_JOY_R_PIN GPIO_PIN_3
#define MC_JOY_U_PORT GPIOE
#define MC_JOY_U_PIN GPIO_PIN_2
#define MC_JOY_D_PORT GPIOE
#define MC_JOY_D_PIN GPIO_PIN_3
#define MC_JOY_C_PORT GPIOE
#define MC_JOY_C_PIN GPIO_PIN_4
#ifndef MC_TOUCH_REVISION_D
#define MC_TOUCH_REVISION_D 0
#endif
#define MC_UART_BAUD 115200u
/* PA5 DAC_OUT2 -> analog module DAC input. Never connect a bare speaker here. */
#ifndef MC_AUDIO_ENABLED
#define MC_AUDIO_ENABLED 1
#endif
#define MC_AUDIO_RATE 16000u
#define MC_AUDIO_PEAK 160u /* Conservative DAC amplitude; verify on the actual module. */
#endif
