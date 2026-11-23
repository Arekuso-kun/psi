#ifndef SI_INPUT_H
#define SI_INPUT_H
#include "game.h"
enum { SI_LEFT=1, SI_RIGHT=2, SI_FIRE=4, SI_VOL_UP=8, SI_VOL_DOWN=16, SI_PAUSE=32, SI_START=64 };
typedef struct {
    uint8_t mask[2], edges;
    uint32_t order[2][2], sequence, remote_at, volume_at;
    uint8_t remote_valid, previous_volume;
} SiInput;
void si_input_init(SiInput *in);
void si_input_local(SiInput *in, uint8_t mask);
void si_input_remote(SiInput *in, uint8_t mask, uint32_t now);
SiControls si_input_poll(SiInput *in, uint32_t now);
/* Strict line protocol: K,hexmask | VOL,0..100 | PING | STATE? */
typedef enum { SI_BAD_COMMAND, SI_KEYS_COMMAND, SI_VOLUME_COMMAND, SI_PING_COMMAND, SI_STATE_COMMAND } SiCommandType;
typedef struct { SiCommandType type; uint8_t value; } SiCommand;
SiCommand si_protocol_parse(const char *line);
#endif
