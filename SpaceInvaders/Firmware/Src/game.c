#include "game.h"
#include <string.h>

static uint32_t random_next(SiGame *g) {
    g->rng = g->rng * 1664525u + 1013904223u;
    return g->rng;
}

unsigned si_game_alive(const SiGame *g) {
    unsigned n = 0, i;
    for (i = 0; i < SI_ENEMIES; ++i) n += g->alive[i] != 0;
    return n;
}

static void wave_begin(SiGame *g) {
    memset(g->alive, 1, sizeof(g->alive));
    memset(g->bullets, 0, sizeof(g->bullets));
    g->enemy_x = 24;
    g->enemy_y = 62;
    g->enemy_dir = 1;
    g->enemy_elapsed = g->shoot_elapsed = 0;
    g->fire_elapsed = 250;
    g->invulnerable = 1000;
    g->player_x = 120000;
    g->state = SI_PLAYING;
}

void si_game_init(SiGame *g, uint32_t seed) {
    memset(g, 0, sizeof(*g));
    g->rng = seed;
    g->volume = 20;
    g->lives = 3;
    g->level = 1;
    g->player_x = 120000;
    g->state = SI_TITLE;
}

static void new_game(SiGame *g) {
    g->score = 0;
    g->clock = 0;
    g->lives = 3;
    g->level = 1;
    wave_begin(g);
    g->sounds |= SI_NEXT_WAVE;
}

void si_game_set_volume(SiGame *g, int volume) {
    if (volume < 0) volume = 0;
    if (volume > 100) volume = 100;
    g->volume = (uint8_t)volume;
}

static void bullet_add(SiGame *g, int x, int y, uint8_t hostile) {
    unsigned i;
    for (i = 0; i < SI_BULLETS; ++i) {
        if (!g->bullets[i].active) {
            g->bullets[i].active = 1;
            g->bullets[i].hostile = hostile;
            g->bullets[i].x = (int16_t)x;
            g->bullets[i].y = y * 1000;
            break;
        }
    }
}

static void finish(SiGame *g) {
    g->state = SI_GAME_OVER;
    if (g->score > g->best) g->best = g->score;
    g->sounds |= SI_END;
}

static void enemy_move(SiGame *g) {
    int i, minx = SI_WIDTH, maxx = 0, maxy = 0;
    for (i = 0; i < SI_ENEMIES; ++i) if (g->alive[i]) {
        int x = g->enemy_x + (i % 6) * 32;
        int y = g->enemy_y + (i / 6) * 22;
        if (x < minx) minx = x;
        if (x + 12 > maxx) maxx = x + 12;
        if (y + 9 > maxy) maxy = y + 9;
    }
    if ((g->enemy_dir > 0 && maxx + 5 >= SI_WIDTH - 8) ||
        (g->enemy_dir < 0 && minx - 5 <= 8)) {
        g->enemy_dir = (int8_t)-g->enemy_dir;
        g->enemy_y += 9;
        maxy += 9;
    } else {
        g->enemy_x += g->enemy_dir * 5;
    }
    if (maxy >= 276) finish(g);
}

static void enemy_shoot(SiGame *g) {
    int col = (int)(random_next(g) % 6), k, row;
    for (k = 0; k < 6; ++k) {
        int c = (col + k) % 6;
        for (row = 2; row >= 0; --row) if (g->alive[row * 6 + c]) {
            bullet_add(g, g->enemy_x + c * 32 + 6, g->enemy_y + row * 22 + 10, 1);
            return;
        }
    }
}

static void bullets_tick(SiGame *g, uint32_t dt) {
    int i, j, player = (int)(g->player_x / 1000);
    for (i = 0; i < SI_BULLETS; ++i) {
        SiBullet *b = &g->bullets[i];
        int y;
        if (!b->active) continue;
        b->y += (b->hostile ? (110 + g->level * 5) : -240) * (int32_t)dt;
        y = (int)(b->y / 1000);
        if (y < 38 || y > 298) { b->active = 0; continue; }
        if (b->hostile) {
            if (y + 5 >= 282 && y <= 292 && b->x >= player - 9 && b->x <= player + 9) {
                b->active = 0;
                if (!g->invulnerable) {
                    --g->lives;
                    g->invulnerable = 1500;
                    g->sounds |= SI_HURT;
                    if (!g->lives) { finish(g); return; }
                }
            }
        } else {
            for (j = 0; j < SI_ENEMIES; ++j) if (g->alive[j]) {
                int ex = g->enemy_x + (j % 6) * 32;
                int ey = g->enemy_y + (j / 6) * 22;
                if (b->x + 1 >= ex && b->x <= ex + 12 && y <= ey + 9 && y + 5 >= ey) {
                    g->alive[j] = 0;
                    b->active = 0;
                    g->score += (uint32_t)(3 - j / 6) * 10u;
                    if (g->score > g->best) g->best = g->score;
                    g->sounds |= SI_HIT;
                    break;
                }
            }
        }
    }
}

void si_game_tick(SiGame *g, SiControls c, uint32_t dt) {
    uint32_t interval;
    uint8_t fire_pressed = c.fire && !g->fire_was_down;
    g->fire_was_down = c.fire;
    si_game_set_volume(g, g->volume + c.volume_delta);
    if (dt > 50) dt = 50; /* A stalled renderer must not skip collisions. */
    if (g->state == SI_TITLE || g->state == SI_GAME_OVER) {
        if (c.start || c.pause || fire_pressed) new_game(g);
        return;
    }
    if (g->state == SI_PLAYING && c.pause) { g->state = SI_PAUSED; return; }
    if (g->state == SI_PAUSED) {
        if (c.pause) g->state = SI_PLAYING;
        return;
    }
    if (g->state == SI_WAVE) {
        g->wave_elapsed += dt;
        if (g->wave_elapsed >= 1000) {
            if (g->level < 99) ++g->level;
            wave_begin(g);
            g->sounds |= SI_NEXT_WAVE;
        }
        return;
    }
    g->clock += dt;
    g->player_x += c.move * 160 * (int32_t)dt;
    if (g->player_x < 16000) g->player_x = 16000;
    if (g->player_x > 224000) g->player_x = 224000;
    g->invulnerable = g->invulnerable > dt ? g->invulnerable - dt : 0;
    if (g->fire_elapsed < 1000) g->fire_elapsed += dt;
    if (c.fire && g->fire_elapsed >= 220) {
        unsigned i, count = 0;
        for (i = 0; i < SI_BULLETS; ++i)
            if (g->bullets[i].active && !g->bullets[i].hostile) ++count;
        if (count < 4) {
            bullet_add(g, (int)(g->player_x / 1000), 277, 0);
            g->fire_elapsed = 0;
            g->sounds |= SI_SHOT;
        }
    }
    interval = 480u - (g->level > 9 ? 9u : g->level) * 35u;
    if (si_game_alive(g) < 5) interval /= 2;
    g->enemy_elapsed += dt;
    if (g->enemy_elapsed >= interval) { g->enemy_elapsed = 0; enemy_move(g); }
    if (g->state != SI_PLAYING) return;
    g->shoot_elapsed += dt;
    interval = 900u - (g->level > 12 ? 12u : g->level) * 45u;
    if (g->shoot_elapsed >= interval) { g->shoot_elapsed = 0; enemy_shoot(g); }
    bullets_tick(g, dt);
    if (g->state == SI_PLAYING && !si_game_alive(g)) {
        g->state = SI_WAVE;
        g->wave_elapsed = 0;
        memset(g->bullets, 0, sizeof(g->bullets));
    }
}

uint8_t si_game_take_sounds(SiGame *g) { uint8_t s = g->sounds; g->sounds = 0; return s; }
const char *si_game_state_name(SiState state) {
    static const char *const names[] = {"title", "playing", "paused", "wave", "game_over"};
    return state <= SI_GAME_OVER ? names[state] : "error";
}
