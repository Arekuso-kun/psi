#include "game.h"
#include "input.h"
#include "render.h"
#ifdef _WIN32
#define API __declspec(dllexport)
#else
#define API
#endif
static SiGame game;
static SiInput input;
static uint16_t pixels[SI_WIDTH*SI_HEIGHT];
static uint32_t now;
API void sim_init(void) { si_game_init(&game,1234567);si_input_init(&input);now=0; }
API void sim_step(unsigned pc,unsigned local,unsigned dt) {
    now+=dt;si_input_remote(&input,(uint8_t)pc,now);si_input_local(&input,(uint8_t)local);
    si_game_tick(&game,si_input_poll(&input,now),dt);(void)si_game_take_sounds(&game);
}
API void sim_volume(int volume) { si_game_set_volume(&game,volume); }
API const uint16_t *sim_frame(void) { si_render(&game,pixels,1);return pixels; }
API unsigned sim_value(unsigned key) {
    switch(key) { case 0:return game.score;case 1:return game.lives;case 2:return game.level;
        case 3:return game.volume;case 4:return game.state;default:return game.best; }
}
