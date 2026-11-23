#include "app.h"
#include "board.h"
#include "board_config.h"
#include "audio.h"
#include "communication.h"
#include "display.h"
#include "joystick.h"
#include "touch.h"

static McApp app;
static McInput input;
static McJoystick joystick;
static McFramer framer;

static void report(void){
    char line[384];
    if(mc_communication_status(&app,line,sizeof(line),MC_AUDIO_ENABLED,mc_board_uart_errors()))mc_board_send(line);
}
static void serial_poll(uint32_t now){
    int byte;unsigned budget=256;McCommand command;
    static uint32_t seen_errors;
    uint32_t errors=mc_board_uart_errors();
    if(errors!=seen_errors){seen_errors=errors;framer.discard=1;framer.length=0;}
    while(budget-- && (byte=mc_board_read_byte())>=0){
        if(!mc_communication_feed(&framer,byte,&command))continue;
        if(command.type==MC_COMMAND_INVALID){mc_board_send("{\"type\":\"error\",\"message\":\"bad_command\"}\n");continue;}
        if(!app.pc_connected){app.pc_connected=1;++app.revision;}
        app.pc_at=now;
        if(command.type==MC_COMMAND_EVENT){
            if(!mc_input_push(&input,command.event.type,command.event.value))mc_board_send("{\"type\":\"error\",\"message\":\"queue_full\"}\n");
        }else if(command.type==MC_COMMAND_PING)mc_board_send("{\"type\":\"pong\"}\n");
        else report();
    }
}
int main(void){
    uint32_t now,last_frame,last_report,last_touch,last_retry,shown=0,reported=0;
    mc_board_init();mc_app_init(&app,MC_TOUCH_REVISION_D);
    mc_input_init(&input);mc_joystick_init(&joystick);
    app.touch_available=(uint8_t)mc_touch_init();mc_audio_init();
    mc_board_send("{\"type\":\"hello\",\"project\":\"CalculatorSTM32\",\"protocol\":1}\n");
    last_frame=last_report=last_touch=last_retry=mc_board_now();
    for(;;){
        McEvent event;unsigned budget=32;
        now=mc_board_now();serial_poll(now);
        mc_joystick_poll(&joystick,&input,mc_board_keys(),now);
        if(now-last_touch>=10){
            McTouch sample;
            uint8_t was_available=app.touch_available;
            int ok=mc_touch_poll(now,&sample);
            app.touch_available=(uint8_t)mc_touch_available();
            if(was_available!=app.touch_available)++app.revision;
            if(ok)mc_input_push(&input,MC_EVENT_TOUCH,sample.down?(1<<24)|((int)sample.x<<12)|sample.y:0);
            else if(app.touch_down)mc_input_push(&input,MC_EVENT_TOUCH,0);
            last_touch=now;
        }
        while(budget-- && mc_input_pop(&input,&event))mc_app_event(&app,event,now);
        if(!app.touch_available && now-last_retry>=5000){
            app.touch_available=(uint8_t)mc_touch_init();++app.revision;last_retry=now;
        }
        mc_app_tick(&app,now);app.dropped=input.dropped;
        mc_audio_update(now,app.click,app.error_click,app.volume);app.click=app.error_click=0;
        mc_board_led(app.pc_connected,app.calculator.error!=MC_OK);
        if(now-last_report>=200 || (reported!=app.revision && now-last_report>=40)){
            report();last_report=now;reported=app.revision;
        }
        if(shown!=app.revision && now-last_frame>=33){
            mc_display_render(&app,mc_board_backbuffer());mc_board_present();last_frame=now;shown=app.revision;
        }
        __WFI();
    }
}
