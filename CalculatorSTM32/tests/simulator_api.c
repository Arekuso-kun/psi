#include "app.h"
#include "display.h"
#include "communication.h"
#include "joystick.h"
#ifdef _WIN32
#define API __declspec(dllexport)
#else
#define API
#endif
static McApp app;
static McInput input;
static McJoystick joystick;
static McFramer framer;
static uint32_t now;
static uint16_t pixels[MC_WIDTH*MC_HEIGHT];
static char state[384];
API void sim_init(void){
    mc_app_init(&app,0);mc_input_init(&input);mc_joystick_init(&joystick);
    framer=(McFramer){{0},0,0};now=0;
}
API void sim_send(const char *text){
    McCommand c;
    while(*text){
        if(mc_communication_feed(&framer,(unsigned char)*text++,&c) && c.type!=MC_COMMAND_INVALID){
            app.pc_connected=1;app.pc_at=now;
            if(c.type==MC_COMMAND_EVENT)mc_input_push(&input,c.event.type,c.event.value);
        }
    }
}
API void sim_touch(int x,int y,int down){
    int rx=3870-15*x,ry=360+11*y;
    if(rx<0)rx=0;
    if(rx>4095)rx=4095;
    if(ry<0)ry=0;
    if(ry>4095)ry=4095;
    mc_input_push(&input,MC_EVENT_TOUCH,down?(1<<24)|(rx<<12)|ry:0);
}
API void sim_step(unsigned local,unsigned dt){
    McEvent e;now+=dt;mc_joystick_poll(&joystick,&input,(uint8_t)local,now);
    while(mc_input_pop(&input,&e))mc_app_event(&app,e,now);
    mc_app_tick(&app,now);app.dropped=input.dropped;
    app.click=app.error_click=0;
}
API const char *sim_status(void){mc_communication_status(&app,state,sizeof(state),0,0);return state;}
API const uint16_t *sim_frame(void){mc_display_render(&app,pixels);return pixels;}
