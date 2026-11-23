#ifndef MC_INPUT_H
#define MC_INPUT_H
#include <stdint.h>
#include "calculator.h"
enum { MC_LEFT=1,MC_RIGHT=2,MC_UP=4,MC_DOWN=8,MC_CONFIRM=16,MC_USER=32 };
typedef enum { MC_EVENT_KEY,MC_EVENT_NAV,MC_EVENT_CONFIRM,MC_EVENT_VOLUME,MC_EVENT_TOUCH } McEventType;
typedef struct { McEventType type;int value; } McEvent;
#define MC_EVENT_CAPACITY 32
typedef struct { McEvent events[MC_EVENT_CAPACITY];uint8_t head,tail;uint32_t dropped; } McInput;
void mc_input_init(McInput *in);
int mc_input_push(McInput *in,McEventType type,int value);
int mc_input_pop(McInput *in,McEvent *event);
#endif
