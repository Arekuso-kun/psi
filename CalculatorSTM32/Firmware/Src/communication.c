#include "communication.h"
#include <stdio.h>
#include <string.h>
McCommand mc_communication_parse(const char *line) {
    McCommand c={MC_COMMAND_INVALID,{MC_EVENT_KEY,0}};
    if(!line)return c;
    if(!strcmp(line,"PING")){c.type=MC_COMMAND_PING;return c;}
    if(!strcmp(line,"STATE?")){c.type=MC_COMMAND_STATE;return c;}
    if(!strncmp(line,"KEY,",4)){
        McKey key=mc_key_parse(line+4);
        if(key==MC_KEY_INVALID)return c;
        c.type=MC_COMMAND_EVENT;c.event.value=key;return c;
    }
    if(!strcmp(line,"OK")){c.type=MC_COMMAND_EVENT;c.event.type=MC_EVENT_CONFIRM;return c;}
    if(!strncmp(line,"NAV,",4) && strlen(line)==5){
        switch(line[4]){case 'L':c.event.value=MC_LEFT;break;case 'R':c.event.value=MC_RIGHT;break;
            case 'U':c.event.value=MC_UP;break;case 'D':c.event.value=MC_DOWN;break;default:return c;}
        c.type=MC_COMMAND_EVENT;c.event.type=MC_EVENT_NAV;return c;
    }
    if(!strncmp(line,"VOL,",4)){
        const char *p=line+4;unsigned n=0,digits=0;
        while(*p){if(*p<'0'||*p>'9'||++digits>3)return c;n=n*10u+(unsigned)(*p++-'0');}
        if(!digits || n>100)return c;
        c.type=MC_COMMAND_EVENT;c.event.type=MC_EVENT_VOLUME;c.event.value=(int)n;
    }
    return c;
}
unsigned mc_communication_status(const McApp *a,char *out,unsigned capacity,int audio,uint32_t uart_errors) {
    const char *page=a->page==MC_PAGE_CALCULATOR?"calculator":a->page==MC_PAGE_SETTINGS?"settings":"calibration";
    int n=snprintf(out,capacity,"{\"type\":\"state\",\"page\":\"%s\",\"expression\":\"%s\",\"result\":\"%s\",\"error\":%u,\"selected\":%u,\"volume\":%u,\"pc\":%u,\"touch\":%u,\"calibrated\":%u,\"cal_step\":%u,\"audio\":%u,\"uart_errors\":%lu,\"dropped\":%lu}\n",
        page,a->calculator.expression,a->calculator.result,(unsigned)a->calculator.error,a->selected,a->volume,
        a->pc_connected,a->touch_available,a->calibration.calibrated,a->calibration_step,audio?1:0,
        (unsigned long)uart_errors,(unsigned long)a->dropped);
    /* Expressions contain only calculator-generated numbers/operators: no quotes or escapes. */
    return n>=0 && (unsigned)n<capacity?(unsigned)n:0;
}
int mc_communication_feed(McFramer *f,int byte,McCommand *command) {
    if(byte=='\r')return 0;
    if(byte=='\n'){
        int ready=0;
        if(f->length || f->discard){
            f->line[f->length]='\0';
            *command=f->discard?(McCommand){MC_COMMAND_INVALID,{MC_EVENT_KEY,0}}:mc_communication_parse(f->line);
            ready=1;
        }
        f->length=f->discard=0;return ready;
    }
    if(!f->discard){
        if(byte<32 || byte>126 || f->length+1u>=sizeof(f->line)){f->discard=1;f->length=0;}
        else f->line[f->length++]=(char)byte;
    }
    return 0;
}
