#ifndef MC_APP_H
#define MC_APP_H
#include "calculator.h"
#include "calibration.h"
#include "input.h"
#define MC_WIDTH 240
#define MC_HEIGHT 320
typedef enum { MC_PAGE_CALCULATOR,MC_PAGE_SETTINGS,MC_PAGE_CALIBRATION } McPage;
typedef struct {
    McCalculator calculator;
    McCalibration calibration;
    McPoint samples[3];
    McPage page;
    uint32_t revision,press_until,pc_at,errors,dropped;
    uint8_t selected,volume,previous_volume,click,error_click,touch_down,calibration_step;
    uint8_t pc_connected,touch_available;
    int8_t pressed;
} McApp;
typedef struct { int16_t x,y,w,h;McKey key;const char *label; } McButton;
void mc_app_init(McApp *app,int revision_d);
int mc_app_event(McApp *app,McEvent event,uint32_t now);
void mc_app_touch(McApp *app,int down,int raw_x,int raw_y,uint32_t now);
void mc_app_tick(McApp *app,uint32_t now);
int mc_app_hit(McPage page,int x,int y);
const McButton *mc_app_button(McPage page,unsigned index);
unsigned mc_app_button_count(McPage page);
void mc_app_set_volume(McApp *app,int volume);
#endif
