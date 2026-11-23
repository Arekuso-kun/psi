#include "display.h"
#include "graphics.h"
#include <stdio.h>
#include <string.h>
#define BG 0x0843
#define PANEL 0x10c6
#define KEY 0x214a
#define WHITE 0xef7d
#define MUTED 0x8412
#define ACCENT 0x55db
#define ORANGE 0xfd2c
#define RED 0xfa8c

static void button(const McApp *a,unsigned index) {
    const McButton *b=mc_app_button(a->page,index);
    uint16_t fill=KEY,ink=WHITE,edge=PANEL;
    int scale=(a->page==MC_PAGE_CALCULATOR && index!=19)?2:1;
    int len=(int)strlen(b->label),x;
    if(b->key==MC_EQUAL){fill=ACCENT;ink=BG;}
    if(b->key==MC_CLEAR)ink=RED;
    if(b->key>=MC_ADD && b->key<=MC_DIV)ink=ORANGE;
    if(a->selected==index)edge=ACCENT;
    if(a->pressed==(int)index){fill=0x42f0;edge=WHITE;}
    if(b->key==MC_CALIBRATE && !a->touch_available)ink=MUTED;
    mc_gfx_rect(b->x,b->y,b->w,b->h,edge);
    mc_gfx_rect(b->x+2,b->y+2,b->w-4,b->h-4,fill);
    x=b->x+(b->w-len*6*scale+scale)/2;
    mc_gfx_text(x,b->y+(b->h-7*scale)/2,b->label,scale,ink);
}
void mc_display_render(const McApp *a,uint16_t *pixels) {
    char buf[48];unsigned i;
    mc_gfx_buffer(pixels);mc_gfx_rect(0,0,240,320,BG);
    if(a->page==MC_PAGE_CALIBRATION){
        McPoint target=mc_calibration_targets[a->calibration_step<3?a->calibration_step:0];
        mc_gfx_center(10,"CALIBRARE TOUCH",1,ACCENT);
        mc_gfx_center(30,"ATINGE CRUCEA APOI RIDICA",1,WHITE);
        mc_gfx_rect(target.x-10,target.y-1,21,3,ACCENT);
        mc_gfx_rect(target.x-1,target.y-10,3,21,ACCENT);
        mc_gfx_rect(target.x-2,target.y-2,5,5,WHITE);
        (void)snprintf(buf,sizeof(buf),"PUNCT %u / 3",a->calibration_step+1);
        mc_gfx_center(168,buf,2,WHITE);
        mc_gfx_center(195,"APASARI SEPARATE",1,MUTED);
        mc_gfx_center(214,"USER / ESC - ANULARE",1,MUTED);
        return;
    }
    if(a->page==MC_PAGE_SETTINGS){
        mc_gfx_text(10,12,"SETARI",2,WHITE);mc_gfx_text(10,42,"VOLUM",1,MUTED);
        (void)snprintf(buf,sizeof(buf),"%u",a->volume);mc_gfx_text(160,42,buf,3,ACCENT);
        mc_gfx_rect(10,81,220,6,KEY);mc_gfx_rect(10,81,220*a->volume/100,6,ACCENT);
        mc_gfx_center(95,a->volume?"CLIC LA APASARE":"SUNET OPRIT",1,MUTED);
        mc_gfx_center(197,a->touch_available?(a->calibration.calibrated?"TOUCH CALIBRAT":"CALIBRARE IN RAM"):"TOUCH INDISPONIBIL",1,MUTED);
    }else{
        size_t n=strlen(a->calculator.expression),start=n>60?n-60:0,first;
        mc_gfx_text(10,13,"CALCULATOR",1,ACCENT);
        mc_gfx_rect(10,37,220,68,PANEL);
        first=n-start>30?30:n-start;
        memcpy(buf,a->calculator.expression+start,first);buf[first]='\0';
        mc_gfx_text(16,44,buf,1,MUTED);
        mc_gfx_text(16,56,a->calculator.expression+start+first,1,MUTED);
        if(a->calculator.error)mc_gfx_text(16,82,mc_error_name(a->calculator.error),1,RED);
        else {
            const char *result=a->calculator.result;
            int scale=strlen(result)>17?1:2;
            int x=224-(int)strlen(result)*6*scale;
            mc_gfx_text(x,78,result,scale,WHITE);
        }
    }
    for(i=0;i<mc_app_button_count(a->page);++i)button(a,i);
    (void)snprintf(buf,sizeof(buf),"VOL %u",a->volume);mc_gfx_text(10,309,buf,1,MUTED);
    mc_gfx_text(88,309,a->touch_available?"TOUCH":"NO TOUCH",1,a->touch_available?ACCENT:RED);
    mc_gfx_text(190,309,a->pc_connected?"PC ON":"LOCAL",1,MUTED);
}
