#include "calculator.h"
#include "app.h"
#include "joystick.h"
#include "communication.h"
#include "display.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

static void expect(const char *expression,double expected){
    double value=123456;
    assert(mc_evaluate(expression,&value)==MC_OK);
    assert(fabs(value-expected)<=1e-12*(1.0+fabs(expected)));
}
static void press(McCalculator *c,const char *text){
    while(*text){char s[2]={*text++,0};assert(mc_calculator_key(c,mc_key_parse(s)));}
}
static void test_evaluation(void){
    double value;
    expect("2+3*4",14);expect("8/4/2",1);expect("-2*-3+0.5",6.5);
    expect("2+-3",-1);expect("2--3",5);expect(".5+2.",2.5);expect(" 2 + 3 * 4 ",14);
    expect("1e3+1e-3",1000.001);expect("0.1+0.2",0.3);expect("-0",0);
    assert(mc_evaluate("1/0",&value)==MC_DIV_ZERO);
    assert(mc_evaluate("1/-0.0",&value)==MC_DIV_ZERO);
    assert(mc_evaluate("1e308*10",&value)==MC_RANGE);
    assert(mc_evaluate("1e-400",&value)==MC_RANGE);
    {
        const char *invalid[]={"","-","1+","1..2","2**3","2(3)","0x10","nan","inf","1e","1e+","1 2","--2","2+abc","2;3"};
        unsigned i;for(i=0;i<sizeof(invalid)/sizeof(invalid[0]);++i)assert(mc_evaluate(invalid[i],&value)==MC_SYNTAX);
    }
}
static void test_editing(void){
    McCalculator c;unsigned i;
    mc_calculator_init(&c);press(&c,"2+3*4=");assert(c.evaluated&&!strcmp(c.result,"14"));
    press(&c,"+1=");assert(!strcmp(c.result,"15"));
    press(&c,"7");assert(!strcmp(c.expression,"7")&&!c.evaluated);
    assert(mc_calculator_key(&c,MC_DOT));assert(!mc_calculator_key(&c,MC_DOT));press(&c,"5=");assert(c.value==7.5);
    assert(mc_calculator_key(&c,MC_SIGN));assert(c.value==-7.5);
    assert(mc_calculator_key(&c,MC_DELETE));assert(!strcmp(c.expression,"")&&!strcmp(c.result,"0"));
    press(&c,"2+");assert(mc_calculator_key(&c,MC_SIGN));press(&c,"3=");assert(c.value==-1);
    mc_calculator_init(&c);press(&c,"2+-3=");assert(c.value==-1);
    mc_calculator_init(&c);press(&c,"1/0=");assert(c.error==MC_DIV_ZERO);
    assert(mc_calculator_key(&c,MC_DELETE));press(&c,"2=");assert(c.value==0.5);
    mc_calculator_init(&c);for(i=0;i<MC_EXPRESSION_MAX;++i)assert(mc_calculator_key(&c,MC_1));
    assert(!mc_calculator_key(&c,MC_1)&&c.error==MC_FULL);
    assert(strlen(c.expression)==MC_EXPRESSION_MAX);assert(mc_calculator_key(&c,MC_DELETE));
    assert(c.error==MC_OK&&strlen(c.expression)==MC_EXPRESSION_MAX-1);
    assert(mc_key_parse(",")==MC_DOT);
}
static void app_key(McApp *a,McKey key){mc_app_event(a,(McEvent){MC_EVENT_KEY,key},100);}
static void tap(McApp *a,int x,int y){
    mc_app_touch(a,1,3870-15*x,360+11*y,100);mc_app_touch(a,0,0,0,140);
}
static void test_sources_and_ui(void){
    McApp a;unsigned i;
    mc_app_init(&a,0);
    tap(&a,36,245); /* physical touch digit 1 */
    assert(!strcmp(a.calculator.expression,"1"));
    app_key(&a,MC_ADD); /* PC keyboard */
    a.selected=13;mc_app_event(&a,(McEvent){MC_EVENT_CONFIRM,0},160); /* joystick digit 2 */
    app_key(&a,MC_EQUAL);assert(a.calculator.value==3);
    mc_app_touch(&a,1,3870-15*36,360+11*245,200);
    for(i=0;i<20;++i)mc_app_touch(&a,1,3870-15*90,360+11*245,200+i);
    assert(!strcmp(a.calculator.expression,"1")); /* drag/hold does not type extra digits */
    mc_app_touch(&a,0,0,0,300);
    app_key(&a,MC_SETTINGS);assert(a.page==MC_PAGE_SETTINGS);
    app_key(&a,MC_ADD);assert(a.volume==30);
    app_key(&a,MC_MUTE);assert(!a.volume);app_key(&a,MC_MUTE);assert(a.volume==30);
    mc_app_set_volume(&a,999);assert(a.volume==100);mc_app_set_volume(&a,-1);assert(!a.volume);
    app_key(&a,MC_BACK);assert(a.page==MC_PAGE_CALCULATOR&&!strcmp(a.calculator.expression,"1"));
    a.selected=0;mc_app_event(&a,(McEvent){MC_EVENT_NAV,MC_UP},0);assert(a.selected==19);
    mc_app_event(&a,(McEvent){MC_EVENT_CONFIRM,0},0);assert(a.page==MC_PAGE_SETTINGS);
    a.pc_connected=1;a.pc_at=0xffffff00u;mc_app_tick(&a,0x100u);assert(a.pc_connected);
    mc_app_tick(&a,0x1000u);assert(!a.pc_connected);
    a.touch_available=0;app_key(&a,MC_CALIBRATE);assert(a.page==MC_PAGE_SETTINGS);
}
static void test_calibration(void){
    McCalibration c;McPoint p[3],mapped;unsigned i;
    mc_calibration_default(&c,0);
    for(i=0;i<3;++i){p[i].x=(int16_t)(3870-15*mc_calibration_targets[i].x);p[i].y=(int16_t)(360+11*mc_calibration_targets[i].y);}
    assert(mc_calibration_solve(&c,p)&&c.calibrated);
    for(i=0;i<3;++i){mapped=mc_calibration_map(&c,p[i].x,p[i].y);assert(mapped.x==mc_calibration_targets[i].x&&mapped.y==mc_calibration_targets[i].y);}
    p[0]=p[1]=p[2]=(McPoint){100,100};assert(!mc_calibration_solve(&c,p));
    mapped=mc_calibration_map(&c,0,4095);assert(mapped.x==239&&mapped.y==319);
    {
        McApp a;mc_app_init(&a,0);app_key(&a,MC_SETTINGS);app_key(&a,MC_CALIBRATE);
        assert(a.page==MC_PAGE_CALIBRATION);
        for(i=0;i<3;++i)tap(&a,mc_calibration_targets[i].x,mc_calibration_targets[i].y);
        assert(a.page==MC_PAGE_SETTINGS&&a.calibration.calibrated);
        app_key(&a,MC_BACK);tap(&a,36,245);assert(!strcmp(a.calculator.expression,"1"));
    }
}
static void test_joystick(void){
    McInput in;McJoystick j;McEvent e;
    mc_input_init(&in);mc_joystick_init(&j);
    mc_joystick_poll(&j,&in,MC_DOWN,100);assert(!mc_input_pop(&in,&e));
    mc_joystick_poll(&j,&in,MC_DOWN,119);assert(!mc_input_pop(&in,&e));
    mc_joystick_poll(&j,&in,MC_DOWN,120);assert(mc_input_pop(&in,&e)&&e.type==MC_EVENT_NAV&&e.value==MC_DOWN);
    mc_joystick_poll(&j,&in,MC_DOWN,519);assert(!mc_input_pop(&in,&e));
    mc_joystick_poll(&j,&in,MC_DOWN,520);assert(mc_input_pop(&in,&e));
    mc_joystick_poll(&j,&in,MC_CONFIRM,600);mc_joystick_poll(&j,&in,MC_CONFIRM,620);
    assert(mc_input_pop(&in,&e)&&e.type==MC_EVENT_CONFIRM);
    mc_joystick_poll(&j,&in,MC_CONFIRM,900);assert(!mc_input_pop(&in,&e));
}
static void test_protocol_and_queue(void){
    McFramer f={{0},0,0};McCommand c;char state[384];McApp a;McInput in;McEvent e;unsigned i;
    const char *bad[]={"KEY,","KEY,abc","KEY,1\nKEY,2","VOL,101","VOL,-1","VOL,","VOL,12x","NAV,X","OK ","PING x"};
    assert(mc_communication_parse("KEY,7").event.value==MC_7);
    assert(mc_communication_parse("KEY,,").event.value==MC_DOT);
    assert(mc_communication_parse("VOL,100").event.value==100);
    for(i=0;i<sizeof(bad)/sizeof(bad[0]);++i)assert(mc_communication_parse(bad[i]).type==MC_COMMAND_INVALID);
    assert(!mc_communication_feed(&f,'K',&c));assert(!mc_communication_feed(&f,'E',&c));
    assert(!mc_communication_feed(&f,'Y',&c));assert(!mc_communication_feed(&f,',',&c));assert(!mc_communication_feed(&f,'3',&c));
    assert(mc_communication_feed(&f,'\n',&c)&&c.type==MC_COMMAND_EVENT&&c.event.value==MC_3);
    for(i=0;i<80;++i)mc_communication_feed(&f,'X',&c);
    assert(mc_communication_feed(&f,'\n',&c)&&c.type==MC_COMMAND_INVALID);
    mc_input_init(&in);for(i=0;i<MC_EVENT_CAPACITY-1;++i)assert(mc_input_push(&in,MC_EVENT_KEY,(int)i));
    assert(!mc_input_push(&in,MC_EVENT_KEY,0)&&in.dropped==1);
    for(i=0;i<MC_EVENT_CAPACITY-1;++i){assert(mc_input_pop(&in,&e));assert(e.value==(int)i);}
    assert(!mc_input_pop(&in,&e));
    mc_app_init(&a,0);assert(mc_communication_status(&a,state,sizeof(state),1,0)>0);
    assert(strstr(state,"\"expression\":\"\"")&&strstr(state,"\"volume\":20"));
    assert(mc_communication_status(&a,state,4,1,0)==0);
}
static void test_renderer(void){
    static uint16_t pixels[MC_WIDTH*MC_HEIGHT+2];McApp a;unsigned page;
    mc_app_init(&a,0);pixels[0]=0x1234;pixels[MC_WIDTH*MC_HEIGHT+1]=0xabcd;
    for(page=0;page<3;++page){a.page=(McPage)page;mc_display_render(&a,pixels+1);assert(pixels[0]==0x1234&&pixels[MC_WIDTH*MC_HEIGHT+1]==0xabcd);}
}
int main(void){
    test_evaluation();test_editing();test_sources_and_ui();test_calibration();test_joystick();test_protocol_and_queue();test_renderer();
    puts("PASS: precedence, decimals, negatives, errors, editing, shared inputs, single touch, calibration, joystick, framing, queue, render bounds");return 0;
}
