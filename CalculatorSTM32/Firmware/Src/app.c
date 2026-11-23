#include "app.h"
#include <string.h>

static const McButton calculator_buttons[]={
 {10,118,52,32,MC_CLEAR,"C"},{66,118,52,32,MC_DELETE,"DEL"},{122,118,52,32,MC_SIGN,"+/-"},{178,118,52,32,MC_DIV,"/"},
 {10,155,52,32,MC_7,"7"},{66,155,52,32,MC_8,"8"},{122,155,52,32,MC_9,"9"},{178,155,52,32,MC_MUL,"X"},
 {10,192,52,32,MC_4,"4"},{66,192,52,32,MC_5,"5"},{122,192,52,32,MC_6,"6"},{178,192,52,32,MC_SUB,"-"},
 {10,229,52,32,MC_1,"1"},{66,229,52,32,MC_2,"2"},{122,229,52,32,MC_3,"3"},{178,229,52,32,MC_ADD,"+"},
 {10,266,52,32,MC_0,"0"},{66,266,52,32,MC_DOT,"."},{122,266,108,32,MC_EQUAL,"="},
 {190,5,40,25,MC_SETTINGS,"SET"}
};
static const McButton settings_buttons[]={
 {10,108,106,34,MC_VOL_DOWN,"-10"},{124,108,106,34,MC_VOL_UP,"+10"},
 {10,158,220,36,MC_MUTE,"MUTE / SUNET"},{10,207,220,36,MC_CALIBRATE,"CALIBRARE TOUCH"},
 {10,256,220,36,MC_BACK,"INAPOI"}
};
const McButton *mc_app_button(McPage page,unsigned i) {
    if(page==MC_PAGE_CALCULATOR && i<20)return calculator_buttons+i;
    if(page==MC_PAGE_SETTINGS && i<5)return settings_buttons+i;
    return 0;
}
unsigned mc_app_button_count(McPage page){return page==MC_PAGE_CALCULATOR?20u:page==MC_PAGE_SETTINGS?5u:0u;}
int mc_app_hit(McPage page,int x,int y) {
    unsigned i;
    for(i=0;i<mc_app_button_count(page);++i){
        const McButton *b=mc_app_button(page,i);
        if(x>=b->x && x<b->x+b->w && y>=b->y && y<b->y+b->h)return (int)i;
    }
    return -1;
}
void mc_app_init(McApp *a,int revision_d) {
    memset(a,0,sizeof(*a));mc_calculator_init(&a->calculator);
    mc_calibration_default(&a->calibration,revision_d);a->volume=20;a->previous_volume=20;a->pressed=-1;
    a->touch_available=1;a->revision=1;
}
void mc_app_set_volume(McApp *a,int volume) {
    if(volume<0)volume=0;
    if(volume>100)volume=100;
    if(a->volume!=(uint8_t)volume){a->volume=(uint8_t)volume;++a->revision;}
}
static void navigate(McApp *a,int direction) {
    int s=a->selected;
    if(a->page==MC_PAGE_CALIBRATION)return;
    if(a->page==MC_PAGE_SETTINGS){
        if(direction==MC_LEFT && s==1)s=0;
        else if(direction==MC_RIGHT && s==0)s=1;
        else if(direction==MC_UP)s=s>=3?s-1:0;
        else if(direction==MC_DOWN)s=s<2?2:(s<4?s+1:4);
    }else if(s==19){if(direction==MC_DOWN)s=0;}
    else {
        int row=s/4,col=s%4;
        if(direction==MC_LEFT){if(row==4)s=s==18?17:s==17?16:16;else if(col>0)--s;}
        if(direction==MC_RIGHT){if(row==4)s=s<18?s+1:18;else if(col<3)++s;}
        if(direction==MC_UP)s=row==0?19:s==18?14:s-4;
        if(direction==MC_DOWN){if(row<3)s+=4;else if(row==3)s=col>=2?18:16+col;}
    }
    if(s!=a->selected){a->selected=(uint8_t)s;++a->revision;}
}
static int key(McApp *a,McKey k,uint32_t now) {
    int accepted=0;McPage before=a->page;
    McError previous_error=a->calculator.error;
    if(k==MC_BACK){
        if(a->page!=MC_PAGE_CALCULATOR){a->page=MC_PAGE_CALCULATOR;a->selected=19;accepted=1;}
        else accepted=mc_calculator_key(&a->calculator,MC_CLEAR);
    }else if(k==MC_SETTINGS){
        a->page=a->page==MC_PAGE_SETTINGS?MC_PAGE_CALCULATOR:MC_PAGE_SETTINGS;a->selected=0;accepted=1;
    }else if(a->page==MC_PAGE_SETTINGS){
        if(k==MC_VOL_UP || k==MC_ADD){mc_app_set_volume(a,a->volume+10);accepted=1;}
        if(k==MC_VOL_DOWN || k==MC_SUB){mc_app_set_volume(a,a->volume-10);accepted=1;}
        if(k==MC_MUTE){
            if(a->volume){a->previous_volume=a->volume;mc_app_set_volume(a,0);}
            else mc_app_set_volume(a,a->previous_volume?a->previous_volume:20);
            accepted=1;
        }
        if(k==MC_CALIBRATE && a->touch_available){a->page=MC_PAGE_CALIBRATION;a->calibration_step=0;accepted=1;}
    }else if(a->page==MC_PAGE_CALCULATOR)accepted=mc_calculator_key(&a->calculator,k);
    if(accepted){
        a->click=1;a->error_click=a->calculator.error!=MC_OK && a->page==MC_PAGE_CALCULATOR;
        a->press_until=now+100;++a->revision;
        if(before==a->page){
            unsigned i;
            for(i=0;i<mc_app_button_count(a->page);++i)if(mc_app_button(a->page,i)->key==k){a->pressed=(int8_t)i;break;}
        }else a->pressed=-1;
    }else if(a->calculator.error!=MC_OK && a->page==MC_PAGE_CALCULATOR){
        a->error_click=a->click=1;++a->errors;++a->revision;
    }
    if(!accepted && previous_error!=a->calculator.error)++a->revision;
    return accepted;
}
int mc_app_event(McApp *a,McEvent event,uint32_t now) {
    const McButton *button;
    switch(event.type){
        case MC_EVENT_NAV:navigate(a,event.value);return 0;
        case MC_EVENT_CONFIRM:
            if(a->page==MC_PAGE_CALIBRATION)return key(a,MC_BACK,now);
            button=mc_app_button(a->page,a->selected);
            return button?key(a,button->key,now):0;
        case MC_EVENT_VOLUME:mc_app_set_volume(a,event.value);return 0;
        case MC_EVENT_TOUCH:
            mc_app_touch(a,(event.value>>24)&1,(event.value>>12)&4095,event.value&4095,now);return 0;
        case MC_EVENT_KEY:
            if(event.value<MC_0 || event.value>=MC_KEY_INVALID)return 0;
            return key(a,(McKey)event.value,now);
        default:return 0;
    }
}
void mc_app_touch(McApp *a,int down,int raw_x,int raw_y,uint32_t now) {
    int rising=down && !a->touch_down;
    a->touch_down=(uint8_t)(down!=0);
    if(!rising)return;
    if(a->page==MC_PAGE_CALIBRATION){
        a->samples[a->calibration_step].x=(int16_t)raw_x;
        a->samples[a->calibration_step].y=(int16_t)raw_y;
        a->click=1;++a->calibration_step;++a->revision;
        if(a->calibration_step==3){
            if(mc_calibration_solve(&a->calibration,a->samples)){a->page=MC_PAGE_SETTINGS;a->selected=4;}
            else {a->calibration_step=0;a->error_click=1;++a->errors;}
        }
    }else{
        McPoint p=mc_calibration_map(&a->calibration,raw_x,raw_y);
        int index=mc_app_hit(a->page,p.x,p.y);
        if(index>=0){a->selected=(uint8_t)index;(void)key(a,mc_app_button(a->page,(unsigned)index)->key,now);}
    }
}
void mc_app_tick(McApp *a,uint32_t now) {
    if(a->pressed>=0 && (int32_t)(now-a->press_until)>=0){a->pressed=-1;++a->revision;}
    if(a->pc_connected && now-a->pc_at>=2500){a->pc_connected=0;++a->revision;}
}
