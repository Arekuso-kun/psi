#ifndef MC_AUDIO_H
#define MC_AUDIO_H
#include <stdint.h>
void mc_audio_init(void);
void mc_audio_update(uint32_t now, uint8_t click, uint8_t error, uint8_t volume);
void mc_audio_irq(void);
#endif
