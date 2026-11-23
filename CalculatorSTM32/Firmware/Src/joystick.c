#include "joystick.h"
#include <string.h>
void mc_joystick_init(McJoystick *j){memset(j,0,sizeof(*j));}
void mc_joystick_poll(McJoystick *j,McInput *input,uint8_t raw,uint32_t now) {
    uint8_t newly,direction=0;
    if(raw!=j->candidate){j->candidate=raw;j->since=now;}
    if(now-j->since<20)return;
    newly=(uint8_t)(raw & ~j->stable);j->stable=raw;
    if(newly&(MC_CONFIRM|MC_USER))mc_input_push(input,MC_EVENT_CONFIRM,0);
    if((raw&(MC_LEFT|MC_RIGHT))==MC_LEFT)direction=MC_LEFT;
    else if((raw&(MC_LEFT|MC_RIGHT))==MC_RIGHT)direction=MC_RIGHT;
    else if((raw&(MC_UP|MC_DOWN))==MC_UP)direction=MC_UP;
    else if((raw&(MC_UP|MC_DOWN))==MC_DOWN)direction=MC_DOWN;
    if(direction && (direction!=j->last_direction || (int32_t)(now-j->repeat_at)>=0)){
        mc_input_push(input,MC_EVENT_NAV,direction);
        j->repeat_at=now+(direction!=j->last_direction?400u:120u);
    }
    j->last_direction=direction;
}
