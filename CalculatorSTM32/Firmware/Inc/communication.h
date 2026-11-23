#ifndef MC_COMMUNICATION_H
#define MC_COMMUNICATION_H
#include "app.h"
typedef enum { MC_COMMAND_INVALID,MC_COMMAND_EVENT,MC_COMMAND_PING,MC_COMMAND_STATE } McCommandType;
typedef struct { McCommandType type;McEvent event; } McCommand;
McCommand mc_communication_parse(const char *line);
unsigned mc_communication_status(const McApp *app,char *out,unsigned capacity,int audio_enabled,uint32_t uart_errors);
/* Bounded stream framing shared by firmware and host tests. */
typedef struct { char line[48];uint8_t length,discard; } McFramer;
int mc_communication_feed(McFramer *framer,int byte,McCommand *command);
#endif
