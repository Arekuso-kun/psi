#include "game.h"
#include "input.h"
#include "render.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static void test_input(void) {
    SiInput in; SiControls c;
    si_input_init(&in);
    si_input_local(&in,SI_LEFT|SI_FIRE);
    c=si_input_poll(&in,0);assert(c.move==-1&&c.fire);
    si_input_remote(&in,SI_RIGHT,100);
    c=si_input_poll(&in,100);assert(c.move==1&&c.fire);
    si_input_remote(&in,0,110);
    c=si_input_poll(&in,110);assert(c.move==-1&&c.fire);
    si_input_remote(&in,SI_RIGHT|SI_FIRE,200);
    c=si_input_poll(&in,549);assert(c.move==1);
    c=si_input_poll(&in,550);assert(c.move==-1&&c.fire&&!in.remote_valid);
    si_input_local(&in,0);c=si_input_poll(&in,551);assert(!c.fire&&!c.move);
    si_input_local(&in,SI_PAUSE|SI_VOL_UP);
    c=si_input_poll(&in,600);assert(c.pause&&c.volume_delta==10);
    c=si_input_poll(&in,616);assert(!c.pause&&!c.volume_delta);
    c=si_input_poll(&in,800);assert(c.volume_delta==10);
    si_input_local(&in,SI_VOL_UP|SI_VOL_DOWN);c=si_input_poll(&in,820);assert(!c.volume_delta);
    si_input_init(&in);si_input_remote(&in,SI_LEFT,0xffffff00u);
    c=si_input_poll(&in,0x20u);assert(c.move==-1);
    c=si_input_poll(&in,0x80u);assert(!c.move); /* tick wrap */
}
static void test_protocol(void) {
    const char *bad[]={"", "K,", "K,80", "K,ff", "K,-1", "K,001", "K,4x", "VOL,", "VOL,-1", "VOL,101", "VOL,1000", "VOL,3x", "PING ", "K,1\nVOL,2"};
    unsigned i;
    assert(si_protocol_parse("K,45").type==SI_KEYS_COMMAND);
    assert(si_protocol_parse("K,45").value==0x45);
    assert(si_protocol_parse("VOL,100").value==100);
    assert(si_protocol_parse("VOL,0").type==SI_VOLUME_COMMAND);
    assert(si_protocol_parse("STATE?").type==SI_STATE_COMMAND);
    assert(si_protocol_parse("PING").type==SI_PING_COMMAND);
    assert(si_protocol_parse(NULL).type==SI_BAD_COMMAND);
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);++i) assert(si_protocol_parse(bad[i]).type==SI_BAD_COMMAND);
}
static void start(SiGame *g) { SiControls c={0};si_game_init(g,42);c.start=1;si_game_tick(g,c,16);assert(g->state==SI_PLAYING); }
static void test_game(void) {
    SiGame g;SiControls c={0};unsigned i;int32_t x;
    start(&g);assert(si_game_alive(&g)==18&&g.lives==3&&g.level==1);
    c.move=-1;for(i=0;i<200;++i)si_game_tick(&g,c,16);assert(g.player_x==16000);
    c.move=1;for(i=0;i<200;++i)si_game_tick(&g,c,16);assert(g.player_x==224000);
    c.move=0;c.pause=1;si_game_tick(&g,c,16);assert(g.state==SI_PAUSED);
    x=g.player_x;c.pause=0;c.move=-1;si_game_tick(&g,c,16);assert(g.player_x==x);
    c.pause=1;si_game_tick(&g,c,16);assert(g.state==SI_PLAYING);
    c=(SiControls){0};
    /* Exact projectile overlap with a live enemy awards points once. */
    g.bullets[0]=(SiBullet){(g.enemy_y+3)*1000,(int16_t)(g.enemy_x+5),1,0};
    g.enemy_elapsed=0;si_game_tick(&g,c,1);assert(!g.alive[0]&&g.score==30);
    si_game_tick(&g,c,1);assert(g.score==30);
    memset(g.alive,0,sizeof(g.alive));si_game_tick(&g,c,16);assert(g.state==SI_WAVE);
    for(i=0;i<63;++i) { si_game_tick(&g,c,16); }
    assert(g.level==2&&g.state==SI_PLAYING&&si_game_alive(&g)==18);
    g.invulnerable=0;g.lives=1;
    g.bullets[0]=(SiBullet){283000,(int16_t)(g.player_x/1000),1,1};
    si_game_tick(&g,c,1);assert(g.state==SI_GAME_OVER&&!g.lives);
    g.fire_was_down=1;c.fire=1;si_game_tick(&g,c,16);assert(g.state==SI_GAME_OVER);
    c.fire=0;si_game_tick(&g,c,16);assert(g.state==SI_GAME_OVER);
    c.start=1;si_game_tick(&g,c,16);assert(g.state==SI_PLAYING&&g.score==0&&g.best==30);
    si_game_set_volume(&g,-200);assert(!g.volume);si_game_set_volume(&g,300);assert(g.volume==100);
    /* Repeated pseudo-random input must preserve game invariants. */
    for(i=0;i<30000;++i) {
        c.move=(int8_t)((i/93)%3)-1;c.fire=1;c.start=g.state==SI_GAME_OVER;c.pause=0;
        si_game_tick(&g,c,16);assert(g.player_x>=16000&&g.player_x<=224000&&g.lives<=3);
    }
}
static void test_render(void) {
    static uint16_t memory[SI_WIDTH*SI_HEIGHT+2];
    SiGame g;unsigned s;
    si_game_init(&g,1);memory[0]=0x1234;memory[SI_WIDTH*SI_HEIGHT+1]=0xabcd;
    for(s=SI_TITLE;s<=SI_GAME_OVER;++s) {
        g.state=(SiState)s;si_render(&g,memory+1,1);
        assert(memory[0]==0x1234&&memory[SI_WIDTH*SI_HEIGHT+1]==0xabcd);
    }
}
int main(void) {
    test_input();test_protocol();test_game();test_render();
    puts("PASS: input arbitration, link timeout, protocol validation, collisions, waves, lives, pause, volume, render bounds, stress run");
    return 0;
}
