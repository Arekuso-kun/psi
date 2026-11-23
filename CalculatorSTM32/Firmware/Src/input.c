#include "input.h"
#include <string.h>
void mc_input_init(McInput *in){memset(in,0,sizeof(*in));}
int mc_input_push(McInput *in,McEventType type,int value) {
    unsigned next=(in->head+1u)%MC_EVENT_CAPACITY;
    if(next==in->tail){++in->dropped;return 0;}
    in->events[in->head].type=type;in->events[in->head].value=value;in->head=(uint8_t)next;return 1;
}
int mc_input_pop(McInput *in,McEvent *event) {
    if(in->head==in->tail)return 0;
    *event=in->events[in->tail];in->tail=(uint8_t)((in->tail+1u)%MC_EVENT_CAPACITY);return 1;
}
