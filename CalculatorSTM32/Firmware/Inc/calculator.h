#ifndef MC_CALCULATOR_H
#define MC_CALCULATOR_H
#include <stdint.h>
#define MC_EXPRESSION_MAX 63
typedef enum {
    MC_0, MC_1, MC_2, MC_3, MC_4, MC_5, MC_6, MC_7, MC_8, MC_9,
    MC_DOT, MC_ADD, MC_SUB, MC_MUL, MC_DIV, MC_EQUAL, MC_CLEAR, MC_DELETE, MC_SIGN,
    MC_SETTINGS, MC_BACK, MC_MUTE, MC_CALIBRATE, MC_VOL_UP, MC_VOL_DOWN, MC_KEY_INVALID
} McKey;
typedef enum { MC_OK, MC_SYNTAX, MC_DIV_ZERO, MC_RANGE, MC_FULL } McError;
typedef struct {
    char expression[MC_EXPRESSION_MAX+1];
    char result[32];
    double value;
    McError error;
    uint8_t evaluated;
} McCalculator;
void mc_calculator_init(McCalculator *c);
int mc_calculator_key(McCalculator *c, McKey key);
McError mc_evaluate(const char *expression, double *result);
const char *mc_error_name(McError error);
McKey mc_key_parse(const char *token);
const char *mc_key_token(McKey key);
#endif
