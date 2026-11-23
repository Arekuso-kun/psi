#ifndef SI_GAME_H
#define SI_GAME_H
#include <stdint.h>

#define SI_WIDTH 240
#define SI_HEIGHT 320
#define SI_ENEMIES 18
#define SI_BULLETS 16

typedef enum { SI_TITLE, SI_PLAYING, SI_PAUSED, SI_WAVE, SI_GAME_OVER } SiState;
enum { SI_SHOT = 1, SI_HIT = 2, SI_HURT = 4, SI_NEXT_WAVE = 8, SI_END = 16 };
typedef struct { int32_t y; int16_t x; uint8_t active, hostile; } SiBullet;
typedef struct {
    int8_t move;
    uint8_t fire, pause, start;
    int8_t volume_delta;
} SiControls;
typedef struct {
    SiState state;
    uint32_t score, best, rng, clock, enemy_elapsed, shoot_elapsed;
    uint32_t fire_elapsed, invulnerable, wave_elapsed;
    int32_t player_x;
    int16_t enemy_x, enemy_y;
    int8_t enemy_dir;
    uint8_t lives, level, volume, sounds, fire_was_down, alive[SI_ENEMIES];
    SiBullet bullets[SI_BULLETS];
} SiGame;

void si_game_init(SiGame *g, uint32_t seed);
void si_game_tick(SiGame *g, SiControls controls, uint32_t elapsed_ms);
void si_game_set_volume(SiGame *g, int volume);
uint8_t si_game_take_sounds(SiGame *g);
unsigned si_game_alive(const SiGame *g);
const char *si_game_state_name(SiState state);
#endif
