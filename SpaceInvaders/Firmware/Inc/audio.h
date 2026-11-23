#ifndef SI_AUDIO_H
#define SI_AUDIO_H
#include <stdint.h>
void si_audio_init(void);
void si_audio_update(uint32_t now, uint8_t sounds, uint8_t volume, uint8_t paused);
void si_audio_irq(void);
#endif
