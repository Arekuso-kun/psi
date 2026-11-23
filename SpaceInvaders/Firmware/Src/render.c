#include "render.h"
#include <stdio.h>

#define BG 0x0843
#define WHITE 0xef7d
#define CYAN 0x4f5f
#define GREEN 0x77e9
#define RED 0xfa8c
#define YELLOW 0xff87
#define DIM 0x52ad
static uint16_t *fb;
/* Five columns per glyph, least significant bit at the top. */
static const uint8_t font[][5] = {
 {0x3e,0x51,0x49,0x45,0x3e},{0,0x42,0x7f,0x40,0},{0x42,0x61,0x51,0x49,0x46},
 {0x21,0x41,0x45,0x4b,0x31},{0x18,0x14,0x12,0x7f,0x10},{0x27,0x45,0x45,0x45,0x39},
 {0x3c,0x4a,0x49,0x49,0x30},{1,0x71,9,5,3},{0x36,0x49,0x49,0x49,0x36},
 {6,0x49,0x49,0x29,0x1e},
 {0x7e,0x11,0x11,0x11,0x7e},{0x7f,0x49,0x49,0x49,0x36},{0x3e,0x41,0x41,0x41,0x22},
 {0x7f,0x41,0x41,0x22,0x1c},{0x7f,0x49,0x49,0x49,0x41},{0x7f,9,9,9,1},
 {0x3e,0x41,0x49,0x49,0x7a},{0x7f,8,8,8,0x7f},{0,0x41,0x7f,0x41,0},
 {0x20,0x40,0x41,0x3f,1},{0x7f,8,0x14,0x22,0x41},{0x7f,0x40,0x40,0x40,0x40},
 {0x7f,2,0x0c,2,0x7f},{0x7f,4,8,0x10,0x7f},{0x3e,0x41,0x41,0x41,0x3e},
 {0x7f,9,9,9,6},{0x3e,0x41,0x51,0x21,0x5e},{0x7f,9,0x19,0x29,0x46},
 {0x46,0x49,0x49,0x49,0x31},{1,1,0x7f,1,1},{0x3f,0x40,0x40,0x40,0x3f},
 {0x1f,0x20,0x40,0x20,0x1f},{0x3f,0x40,0x38,0x40,0x3f},{0x63,0x14,8,0x14,0x63},
 {7,8,0x70,8,7},{0x61,0x51,0x49,0x45,0x43}
};
static void rect(int x, int y, int w, int h, uint16_t color) {
    int xx, yy;
    for (yy = y < 0 ? 0 : y; yy < y+h && yy < SI_HEIGHT; ++yy)
        for (xx = x < 0 ? 0 : x; xx < x+w && xx < SI_WIDTH; ++xx)
            fb[yy * SI_WIDTH + xx] = color;
}
static void text(int x, int y, const char *s, int scale, uint16_t color) {
    while (*s) {
        int i, row, index = -1;
        char ch = *s++;
        if (ch >= '0' && ch <= '9') index = ch-'0';
        if (ch >= 'A' && ch <= 'Z') index = ch-'A'+10;
        if (index >= 0) {
            for (i = 0; i < 5; ++i) for (row = 0; row < 7; ++row)
                if (font[index][i] & (1u << row)) rect(x+i*scale,y+row*scale,scale,scale,color);
        } else if (ch == '-') rect(x,y+3*scale,5*scale,scale,color);
        else if (ch == '+') { rect(x,y+3*scale,5*scale,scale,color); rect(x+2*scale,y+scale,scale,5*scale,color); }
        else if (ch == '/') { for(i=0;i<5;++i) rect(x+i*scale,y+(5-i)*scale,scale,scale,color); }
        else if (ch == ':') { rect(x+2*scale,y+2*scale,scale,scale,color); rect(x+2*scale,y+5*scale,scale,scale,color); }
        x += 6*scale;
    }
}
static void center(int y, const char *s, int scale, uint16_t color) {
    const char *p = s; int len = 0;
    while (*p++) ++len;
    text((SI_WIDTH-len*6*scale+scale)/2,y,s,scale,color);
}
static void invader(int x, int y, uint16_t color, int frame) {
    static const uint16_t sprite[2][9] = {
        {0x084,0x048,0x1fc,0x3b6,0x7ff,0x5fd,0x504,0x088,0x110},
        {0x084,0x448,0x5fd,0x7b7,0x3fe,0x1fc,0x108,0x204,0x402}
    };
    int row, bit;
    for(row=0;row<9;++row) for(bit=0;bit<11;++bit)
        if(sprite[frame & 1][row] & (1u<<bit)) rect(x+bit,y+row,1,1,color);
}
static void ship(int x, int y) {
    rect(x-1,y,3,3,GREEN); rect(x-5,y+3,11,3,GREEN); rect(x-8,y+6,17,4,GREEN);
    rect(x-3,y+6,7,2,CYAN);
}
void si_render(const SiGame *g, uint16_t *pixels, uint8_t connected) {
    int i;
    char buf[40];
    fb = pixels;
    rect(0,0,SI_WIDTH,SI_HEIGHT,BG);
    for(i=0;i<48;++i) {
        int x=(i*73+19)%SI_WIDTH, y=40+(i*47+13)%254;
        rect(x,y,1,1,(i%3)?0x18c7:0x39cc);
    }
    text(8,7,"SCORE",1,DIM); text(108,7,"WAVE",1,DIM); text(185,7,"LIVES",1,DIM);
    (void)snprintf(buf,sizeof(buf),"%06lu",(unsigned long)g->score); text(8,19,buf,1,WHITE);
    (void)snprintf(buf,sizeof(buf),"%02u",g->level); text(115,19,buf,1,CYAN);
    for(i=0;i<g->lives;++i) rect(187+i*13,19,8,6,GREEN);
    rect(8,34,224,1,0x2129);
    rect(8,301,224,1,0x2129);
    (void)snprintf(buf,sizeof(buf),"VOL %u",g->volume); text(8,309,buf,1,DIM);
    text(156,309,connected?"PC ONLINE":"LOCAL",1,connected?CYAN:DIM);
    if(g->state==SI_TITLE) {
        center(64,"SPACE",3,CYAN); center(92,"INVADERS",3,WHITE);
        for(i=0;i<5;++i) invader(40+i*38,139,i%2?GREEN:CYAN,i);
        center(170,"USER / SPACE TO START",1,GREEN);
        center(200,"JOYSTICK OR A / D",1,WHITE);
        center(216,"USER OR SPACE - FIRE",1,WHITE);
        center(237,"UP / DOWN - VOLUME",1,DIM);
        center(253,"PRESS / P - PAUSE",1,DIM);
        return;
    }
    for(i=0;i<SI_ENEMIES;++i) if(g->alive[i])
        invader(g->enemy_x+(i%6)*32,g->enemy_y+(i/6)*22,
                i<6?CYAN:(i<12?GREEN:YELLOW),(int)(g->clock/350));
    for(i=0;i<SI_BULLETS;++i) if(g->bullets[i].active)
        rect(g->bullets[i].x,(int)(g->bullets[i].y/1000),2,5,g->bullets[i].hostile?RED:YELLOW);
    if(!g->invulnerable || ((g->clock/100)&1)) ship((int)(g->player_x/1000),282);
    if(g->state==SI_PAUSED || g->state==SI_GAME_OVER || g->state==SI_WAVE) {
        rect(18,135,204,79,0x10a5); rect(18,135,204,2,CYAN);
        center(150,g->state==SI_PAUSED?"PAUSED":(g->state==SI_WAVE?"WAVE CLEAR":"GAME OVER"),2,
               g->state==SI_GAME_OVER?RED:CYAN);
        center(184,g->state==SI_PAUSED?"PRESS / P TO RESUME":(g->state==SI_WAVE?"GET READY":"USER / ENTER TO START"),1,WHITE);
        if(g->state==SI_GAME_OVER) {
            (void)snprintf(buf,sizeof(buf),"BEST %lu",(unsigned long)g->best);
            center(227,buf,1,YELLOW);
        }
    }
}
