#include "board.h"
#include "board_config.h"
#include "audio.h"
#include "input.h"
#include "render.h"
#include <stdio.h>

static SiGame game;
static SiInput input;

static void report(void) {
    char line[224];
    (void)snprintf(line,sizeof(line),
        "{\"type\":\"state\",\"state\":\"%s\",\"score\":%lu,\"best\":%lu,\"lives\":%u,\"level\":%u,\"volume\":%u,\"pc\":%u,\"audio\":%u,\"uart_errors\":%lu}\n",
        si_game_state_name(game.state),(unsigned long)game.score,(unsigned long)game.best,
        game.lives,game.level,game.volume,input.remote_valid,SI_AUDIO_ENABLED,(unsigned long)si_board_uart_errors());
    (void)si_board_send(line);
}

static void serial_poll(uint32_t now) {
    static char line[48];
    static unsigned length;
    static uint8_t discard;
    static uint32_t errors_seen;
    int byte;
    unsigned budget=256;
    if(si_board_uart_errors()!=errors_seen) {
        errors_seen=si_board_uart_errors(); discard=1; length=0;
    }
    while(budget-- && (byte=si_board_read_byte())>=0) {
        if(byte=='\r') continue;
        if(byte=='\n') {
            if(!discard && length) {
                SiCommand c;
                line[length]='\0'; c=si_protocol_parse(line);
                switch(c.type) {
                    case SI_KEYS_COMMAND: si_input_remote(&input,c.value,now); break;
                    case SI_VOLUME_COMMAND: si_game_set_volume(&game,c.value); report(); break;
                    case SI_PING_COMMAND: (void)si_board_send("{\"type\":\"pong\"}\n"); break;
                    case SI_STATE_COMMAND: report(); break;
                    default: (void)si_board_send("{\"type\":\"error\",\"message\":\"bad_command\"}\n"); break;
                }
            }
            length=0; discard=0;
        } else if(!discard) {
            if(byte<32 || byte>126 || length+1>=sizeof(line)) { discard=1; length=0; }
            else line[length++]=(char)byte;
        }
    }
}

int main(void) {
    uint32_t last_tick, last_frame, last_report, now;
    si_board_init();
    si_input_init(&input); si_game_init(&game,si_board_now()^0x5eed1234u);
    si_audio_init();
    (void)si_board_send("{\"type\":\"hello\",\"project\":\"SpaceInvaders\",\"protocol\":1}\n");
    last_tick=last_frame=last_report=si_board_now();
    for(;;) {
        now=si_board_now();
        serial_poll(now);
        si_input_local(&input,si_board_keys());
        if(now-last_tick>=16) {
            SiControls c=si_input_poll(&input,now);
            si_game_tick(&game,c,now-last_tick); last_tick=now;
            si_audio_update(now,si_game_take_sounds(&game),game.volume,game.state==SI_PAUSED);
            si_board_led(input.remote_valid,game.invulnerable && game.state==SI_PLAYING);
        }
        if(now-last_report>=200) { report(); last_report=now; }
        if(now-last_frame>=33) {
            si_render(&game,si_board_backbuffer(),input.remote_valid);
            si_board_present(); last_frame=now;
        }
        __WFI();
    }
}
